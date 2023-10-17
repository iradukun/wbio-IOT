#include "WioTMobility.h"
#include "DataManager.h"
#include <memory>
#include <chrono>
#include <fmt/format.h>
#include <chrono>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
#include <ctime>
#include <sstream>
#include <iostream>
#include <fstream>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

using namespace std;
constexpr int InitialQueueSize = 256;
CDataManager::CDataManager()
    : m_bWantStop(false)
    , m_zServers("34.197.55.103:5002")
    , m_zClientId("WioTMobility")
    , m_zTopicVehicleData("kafka-iot-vehicle-data-topic")
    , m_zTopicVideo("kafka-iot-video-record-topic")
    , m_rkTopicVehicleData(nullptr)
    , m_rkTopicVideo(nullptr)
{
}

CDataManager::~CDataManager()
{
    PersistVehicleData();
}

void CDataManager::PersistVehicleData() {
    fstream outputFile;
    outputFile.open(m_zVehicleFileName, ios::out);

    for (auto it : mapImeiVehicle) {
        outputFile << it.first << "," << it.second.vehicleId << "," << it.second.vehicle_uuid << "\n";
    }

    outputFile.close();
}


bool CDataManager::Start()
{
    LoadVehicleData();
    
    m_bWantStop = false;
    struct TLauncher
    {
        CDataManager* pManager;

        TLauncher(CDataManager* __pManager) : pManager(__pManager)
        {}

        void operator()()
        {
            pManager->Thread();
        }
    } launcher(this);

    m_Thread = boost::thread{ launcher };

    struct TCLauncher
    {
        CDataManager* pManager;

        TCLauncher(CDataManager* __pManager) : pManager(__pManager)
        {}

        void operator()()
        {
            pManager->ConsumerThread();
        }
    } tclauncher(this);

    m_ConsumerThread = boost::thread{tclauncher};
    
    return true;
}

void CDataManager::LoadVehicleData() {
    fstream dataFile;
    dataFile.open(m_zVehicleFileName, ios::in);

    //load vehicle data
    if (dataFile.is_open()) {
        while (!dataFile.eof()) {

            VehicleImeiData vehicleImeiData;
            string line;

            getline(dataFile, line, '\n');
            
            char* ptr = strtok(const_cast<char*>(line.c_str()), ",");
            
            string imeiNumber(ptr);
            
            ptr = strtok(NULL, ",");
            vehicleImeiData.vehicleId = atoi(ptr);
            
            ptr = strtok(NULL, ",");
            vehicleImeiData.vehicle_uuid = string(ptr);

            mapImeiVehicle[imeiNumber] = vehicleImeiData;
        }
    }

    dataFile.close();
}

bool CDataManager::Stop()
{
    LogInf("WioT", "[DATAMAN] Received signal to quit");
    m_bWantStop = true;
    for (;;)
    {
        if (m_Thread.timed_join(boost::chrono::seconds(WaitThreadTimeout)))
        {
            break;
        }
        LogWarn("WioT", "[DATAMAN] Waiting for thread to stop...");
    }

    for (;;)
    {
        if (m_ConsumerThread.timed_join(boost::chrono::seconds(WaitThreadTimeout)))
        {
            break;
        }
        LogWarn("WioT", "[DATAMAN] Waiting for thread to stop...");
    }
    return true;
}

void CDataManager::SetServers(const char* zServers)
{
    m_zServers = zServers;
}

void CDataManager::SetTopicVehicle(const char* zTopic)
{
    m_zTopicVehicleData = zTopic;
}

void CDataManager::SetTopicVideo(const char* zTopic)
{
    m_zTopicVideo = zTopic;
}

void CDataManager::SetVehicleFileName(const char* zFileName) {
    m_zVehicleFileName = zFileName;
}

void CDataManager::SetClientId(const char* zClientId)
{
    m_zClientId = zClientId;
}

void CDataManager::ErrorCallback(rd_kafka_t* rk, int err, const char* reason, void* opaque)
{
    LogErr("WioT", "[DATAMAN] Get error %d : %s", err, reason);
}

void CDataManager::LogCallback(const rd_kafka_t* rk, int level, const char* fac, const char* buf)
{
    LogInf("WioT", "[DATAMAN] [%s] Info level %d : %s", fac, level, buf);
}

CDataManager& CDataManager::GetInstance()
{
    static CDataManager s_Instance;
    return s_Instance;
}

void CDataManager::Thread()
{
    LogInf("WioT", "[DATAMAN] Entering thread");

    rd_kafka_t* rk = nullptr;
    rd_kafka_conf_t* conf = nullptr;
    rd_kafka_topic_t* vehicle_topic = nullptr;
    rd_kafka_topic_conf_t* vehicle_topic_conf = nullptr;
    rd_kafka_topic_t* video_topic = nullptr;
    rd_kafka_topic_conf_t* video_topic_conf = nullptr;
    if (InitProducer(rk, conf) &&
        InitTopic(rk, m_zTopicVehicleData.c_str(), vehicle_topic, vehicle_topic_conf) &&
        InitTopic(rk, m_zTopicVideo.c_str(), video_topic, video_topic_conf))
    {
        {
            boost::mutex::scoped_lock lock(m_Mutex);
            m_rkTopicVehicleData = vehicle_topic;
            m_rkTopicVideo = video_topic;
        }

        while (!m_bWantStop)
        {
            ConsumeMessage(rk);
        }

        int max_wait_time = 30;
        int nb_msg_in_queue;
        while ((nb_msg_in_queue = rd_kafka_outq_len(rk)) > 0)
        {
            if (--max_wait_time <= 0)
            {
                LogErr("WioT", "[DATAMAN] Still %d message in kafka queue.", nb_msg_in_queue);
                break;
            }
            /* Wait for it to be sent (and possibly acked) */
            rd_kafka_flush(rk, 1000);
        }
        boost::mutex::scoped_lock lock(m_Mutex);
        m_rkTopicVehicleData = nullptr;
        m_rkTopicVideo = nullptr;
    }

    if(video_topic_conf)
    {
        rd_kafka_topic_conf_destroy(video_topic_conf);
    }
    if (vehicle_topic)
    {
        rd_kafka_topic_destroy(vehicle_topic);
    }
    if (video_topic)
    {
        rd_kafka_topic_destroy(video_topic);
    }
    if (conf)
    {
        rd_kafka_conf_destroy(conf);
    }
    if (rk)
    {
        rd_kafka_destroy(rk);
    }

    LogInf("WioT", "[DATAMAN] Leaving thread");
}

void CDataManager::ConsumerThread() {

    LogInf("WioT", "[DATAMAN] Entering Kafka consumer thread");
    
    char errstr[512];
    rd_kafka_t* rk = nullptr;
    rd_kafka_topic_partition_list_t* topics = nullptr;
    rd_kafka_conf_t* conf = nullptr;

    do {
        conf = rd_kafka_conf_new();
        if (!conf) {
            LogErr("WioT", "[DATAMAN] Cannot create a Kfaka configuration for consumer.");
            break;
        }
        
        if (rd_kafka_conf_set(conf, "bootstrap.servers", m_zServers.c_str(),
            //        if (rd_kafka_conf_set(conf, "advertised.listeners", m_zServers.c_str(),
            errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
        {
            LogErr("WioT", "[DATAMAN] Failed to configure servers for kafka consumer.\n%s", errstr);
            break;
        }

        rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr));
        if (!rk) {
            LogErr("WioT", "[DATAMAN] Failed to create kafka consumer.\n%s", errstr);
            break;
        }
        topics = rd_kafka_topic_partition_list_new(0);
        if (!topics) {
            LogErr("WioT", "[DATAMAN] Failed to create kafka topic list.\n%s", errstr);
            break;
        }

        if (!rd_kafka_topic_partition_list_add(topics, "hardware_events", RD_KAFKA_PARTITION_UA)) {
            LogErr("WioT", "[DATAMAN] Failed to add hardware_events topic for consumption.\n");
            break;
        }

        if (RD_KAFKA_RESP_ERR_NO_ERROR != rd_kafka_subscribe(rk, topics)) {
            LogErr("WioT", "[DATAMAN] Failed to subscribe consumer topic.\n");
            break;
        }

        while (1) {
            rd_kafka_message_t* msg = rd_kafka_consumer_poll(rk, 1000);
            if (msg) {
                if (msg->err) {
                    LogErr("WioT", "Error consuming message: %s\n", rd_kafka_message_errstr(msg));
                    rd_kafka_message_destroy(msg);
                    break;
                }
                else {
                    string message((char*)msg->payload, (int)msg->len);
                    boost::property_tree::ptree pt;
                    std::istringstream iss(message);
                    boost::property_tree::read_json(iss, pt);

                    string action = pt.get<string>("action");

                    if (!action.compare("link_hardware")) {
                        VehicleImeiData vehicleImeiData;
                        boost::property_tree::ptree::const_assoc_iterator i_pts = pt.find("data");
                        if (i_pts != pt.not_found()) {
                            const boost::property_tree::ptree& pts = (*i_pts).second;

                            vehicleImeiData.vehicleId = pts.get<int>("vehicle_id");
                            vehicleImeiData.vehicle_uuid = pts.get<string>("vehicle_uuid");
                            string imeiNumber = pts.get<string>("imei_number");

                            mapImeiVehicle[imeiNumber] = vehicleImeiData;
                        }
                    }
                    else if(!action.compare("unlink_hardware"))
                    {
                        boost::property_tree::ptree::const_assoc_iterator i_pts = pt.find("data");
                        if (i_pts != pt.not_found()) {
                            const boost::property_tree::ptree& pts = (*i_pts).second;

                            string imeiNumber = pts.get<string>("imei_number");
                            if (mapImeiVehicle.find(imeiNumber) != mapImeiVehicle.end()) {
                                mapImeiVehicle.erase(imeiNumber);
                            }
                        }
                    }
                }

                rd_kafka_message_destroy(msg);
            }
        }
    } while (false);
    

    if (conf)
    {
        rd_kafka_conf_destroy(conf);
    }
    
    if (topics) {
        rd_kafka_topic_partition_list_destroy(topics);
    }

    if (rk)
    {
        rd_kafka_destroy(rk);
    }
    LogInf("WioT", "[DATAMAN] Leaving KAfka consumer thread");
}

bool CDataManager::InitProducer(rd_kafka_t*& rk, rd_kafka_conf_t*& conf)
{
	char errstr[512];
    
    rk = nullptr;
    conf = nullptr;
    do
    {
        conf = rd_kafka_conf_new();
        if (!conf)
        {
            LogErr("WioT", "[DATAMAN] Cannot create a Kfaka configuration.");
            break;
        }

        if (rd_kafka_conf_set(conf, "client.id", m_zClientId.c_str(),
            errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
        {
            LogErr("WioT", "[DATAMAN] Failed to configure client id.\n%s", errstr);
            break;
        }

        if (rd_kafka_conf_set(conf, "bootstrap.servers", m_zServers.c_str(),
//        if (rd_kafka_conf_set(conf, "advertised.listeners", m_zServers.c_str(),
            errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
        {
            LogErr("WioT", "[DATAMAN] Failed to configure servers.\n%s", errstr);
            break;
        }

        rd_kafka_conf_set_error_cb(conf, ErrorCallback);
        rd_kafka_conf_set_log_cb(conf, LogCallback);

        /* Create Kafka producer handle */
        rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf,
            errstr, sizeof(errstr));
        if (!rk)
        {
            LogErr("WioT", "[DATAMAN] Failed to create new producer.\n%s", errstr);
            break;
        }

        return true;
    } while (false);

    if (conf)
    {
        rd_kafka_conf_destroy(conf);
        conf = nullptr;
    }
    if (rk)
    {
        rd_kafka_destroy(rk);
        rk = nullptr;
    }

    return true;
}

bool CDataManager::InitTopic(rd_kafka_t* rk, const char* zTopicName, rd_kafka_topic_t*& topic, rd_kafka_topic_conf_t*& topic_conf)
{
    char errstr[512];

    topic = nullptr;
    topic_conf = nullptr;
    do
    {
        topic_conf = rd_kafka_topic_conf_new();
        if (!topic_conf)
        {
            LogErr("WioT", "[DATAMAN] Cannot create a Kfaka topic configuration.");
            break;
        }

        if (rd_kafka_topic_conf_set(topic_conf, "acks", "all",
            errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
        {
            LogErr("WioT", "[DATAMAN] Failed to configure topic's acks.\n%s", errstr);
            break;
        }

        topic = rd_kafka_topic_new(rk, zTopicName, topic_conf);
        if (!topic)
        {
            LogErr("WioT", "[DATAMAN] Failed to create new topic.\n%s", errstr);
            break;
        }

        return true;
    } while (false);

    if (topic)
    {
        rd_kafka_topic_destroy(topic);
        topic = nullptr;
    }
    if (topic_conf)
    {
        rd_kafka_topic_conf_destroy(topic_conf);
        topic_conf = nullptr;
    }

    return true;
}

void CDataManager::ConsumeMessage(rd_kafka_t* rk)
{
    while (!m_bWantStop)
    {
        rd_kafka_poll(rk, 500);
    }
}
/*
static void BuildSensorsPayload(const std::string& DeviceId, const std::string &zDate, const CArrayData& ExtraData, std::string& payload)
{
    if (ExtraData.empty())
    {
        return;
    }
    auto builder = bsoncxx::builder::stream::document{};
    auto context = builder
        << "action" << "iot-sensor-data"
        << "data" << open_document
            << "imei_number" << DeviceId
            << "utc_time" << zDate;

    for (const auto& data : ExtraData)
    {
        context << data.Name << data.Value.ToString();
    }

    bsoncxx::document::value value = context << close_document << finalize;

    payload = bsoncxx::to_json(value);
}
*/

static void BuildVideoPayload(const std::string& DeviceId, const TVideoRecordState& VideoState, std::string& payload)
{
    const auto videoDateMs = std::chrono::milliseconds(VideoState.Timestamp);
    const auto videoDate = std::chrono::time_point<std::chrono::system_clock>(videoDateMs);
    const std::string zDate = fmt::format("{0:%FT%H:%M}:{1:%S}Z", videoDate, std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(videoDateMs));

    auto builder = bsoncxx::builder::stream::document{};
    auto context = builder
        << "action" << "iot-video-record"
        << "data" << open_document
        << "imei_number" << DeviceId
        << "type" << VideoState.zType // may be "video" or "photo"
        << "state" << VideoState.zState
        << "timestamp_begin" << zDate
        << "duration" << VideoState.Duration // in second
        << "trigger" << VideoState.zTrigger // may be "DIN1" "DIN2" "Crash" "UserReq"
        << "progress" << VideoState.Progress // in %%
        << "current_length" << VideoState.lLength; // in bytes

    if (VideoState.ulRequestId != NoRequestId)
    {
        context << "req_id" << VideoState.ulRequestId;
    }
    if (!VideoState.zUrl.empty())
    {
        context << "url" << VideoState.zUrl;
    }

    bsoncxx::document::value value = context << close_document << finalize;

    payload = bsoncxx::to_json(value);
}

void CDataManager::BuildVehiclePayload(const std::string& DeviceId, const std::string& zDate, const TGPSdata& GpsData, const CArrayData& ExtraData, std::string& payload)
{
    VehicleImeiData vehicleImeiData;
    if (mapImeiVehicle.find(DeviceId) != mapImeiVehicle.end()) {
        vehicleImeiData = mapImeiVehicle[DeviceId];
    }
    long currentTime = (long)time(0);
    ostringstream oss;
    oss << currentTime << "_" << vehicleImeiData.vehicleId;
    
    auto builder = bsoncxx::builder::stream::document{};
    builder << "action" << "iot-device-data";
    auto builder1 = bsoncxx::builder::stream::document{};
    auto builder2 = bsoncxx::builder::stream::document{};
    auto builder3 = bsoncxx::builder::stream::document{};

    if (GpsData.battery_charging != TGPSdata::EBatteryCharging::NotAvailable) {
        builder3 << "isCharging" << (GpsData.battery_charging == TGPSdata::EBatteryCharging::Charging);
    }
    if (GpsData.battery_level > 0)
    {
        builder3 << "batterylevel" << GpsData.battery_level;
    }

   

    auto builder4 = bsoncxx::builder::stream::document{};
    builder4 << "altitude" << GpsData.altitude
        << "heading" << GpsData.angle
        << "latitude" << GpsData.latitude
        << "longitude" << GpsData.longitude
        << "satellites" << GpsData.satellites
        << "speed" << GpsData.speed;

    builder2 << "battery" << builder3
        << "coords" << builder4
        << "lat" << GpsData.latitude
        << "lng" << GpsData.longitude;

    builder1 << "gps_data" << builder2;
   
    auto builder5 = bsoncxx::builder::stream::document{};
    for (const auto& data : ExtraData)
    {
        builder5 << data.Name << data.Value.ToString();
    }
    builder1 << "sensor_Data" << builder2;
    builder1 << "imei_number" << DeviceId
        << "iot" << true
        << "utc_time" << zDate
        << "timestamp" << currentTime
        << "timestamp_vid" << oss.str()
        << "vehicle_id" << vehicleImeiData.vehicleId
        << "vehicle_uuid" << vehicleImeiData.vehicle_uuid;
    builder << "data" << builder1
            << "timestamp" << currentTime;

    payload = bsoncxx::to_json(builder.view());
}

bool CDataManager::PushData(const std::string& DeviceId, const TGPSdata& GpsData, const CArrayData& ExtraData)
{
    if (!m_rkTopicVehicleData)
    {
        return false;
    }

    const auto dataDateMs = std::chrono::milliseconds(GpsData.timestamp);
    const auto dataDate = std::chrono::time_point<std::chrono::system_clock>(dataDateMs);
    const std::string zDate = fmt::format("{0:%FT%H:%M}:{1:%S}Z", dataDate, std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(dataDateMs));

    std::string payload_vehicle;

    BuildVehiclePayload(DeviceId, zDate, GpsData, ExtraData, payload_vehicle);
    LogInf("WioT", "[DATAMAN] Sending payload Vehicle :\n%s", payload_vehicle.c_str());

#if 0
    FILE * file = fopen("payload.txt", "at");
    if (file)
    {
        fprintf(file, "%s\n", payload_sensors.c_str());
        fclose(file);
    }
#endif
    bool bImeiFound = false;
    VehicleImeiData vehicleImeiData;
    if (mapImeiVehicle.find(DeviceId) != mapImeiVehicle.end()) {
        bImeiFound = true;
        vehicleImeiData = mapImeiVehicle[DeviceId];
    }

    int ret;
    {
        boost::mutex::scoped_lock lock(m_Mutex);
        ret = rd_kafka_produce(m_rkTopicVehicleData, bImeiFound? RD_KAFKA_PARTITION_UA : 0,
            RD_KAFKA_MSG_F_COPY,
            const_cast<char*>(payload_vehicle.c_str()), payload_vehicle.length(),
            bImeiFound? vehicleImeiData.vehicle_uuid.c_str() : nullptr,
            bImeiFound ? vehicleImeiData.vehicle_uuid.length() : 0,
            NULL);
    }
    if (ret == -1)
    {
        LogErr("WioT", "[DATAMAN] Failed to produce to topic.\n%s\n",
            rd_kafka_err2str(rd_kafka_errno2err(errno)));
        return false;
    }

    LogDebug("WioT", "[DATAMAN] Sending payload vehicle :\n%s", payload_vehicle.c_str());
    return true;
}

bool CDataManager::PushVideo(const std::string& DeviceId, const TVideoRecordState& VideoState)
{
    if (!m_rkTopicVideo)
    {
        return false;
    }

    std::string payload_video;

    BuildVideoPayload(DeviceId, VideoState, payload_video);

    int ret;
    {
        boost::mutex::scoped_lock lock(m_Mutex);
        ret = rd_kafka_produce(m_rkTopicVideo, RD_KAFKA_PARTITION_UA,
            RD_KAFKA_MSG_F_COPY,
            const_cast<char*>(payload_video.c_str()), payload_video.length(),
            nullptr, 0,
            NULL);
    }
    if (ret == -1)
    {
        LogErr("WioT", "[DATAMAN] Failed to produce to topic.\n%s\n",
            rd_kafka_err2str(rd_kafka_errno2err(errno)));
        return false;
    }

    LogDebug("WioT", "[DATAMAN] Sending payload Video :\n%s", payload_video.c_str());
    return true;
}
