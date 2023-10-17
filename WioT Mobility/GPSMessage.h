#pragma once
#include <cstdint>
#include <string>
#include <stdexcept>

class CGPSMessage : public IDataMessage
{
public:
    CGPSMessage(const std::string& DeviceId, const TGPSdata& GpsData, const CArrayData& ExtraData)
        : m_DeviceId(DeviceId)
        , m_GpsData(GpsData)
        , m_ExtraData(ExtraData)
    {}
    virtual ~CGPSMessage()
    {}

    bool Process(mongocxx::database& db) override
    {
        if (m_GpsData.longitude != 0. && m_GpsData.latitude != 0.)
        {
            if (!ReplaceDeviceGPS(db))
            {
                return false;
            }
        }
        else
        {
            if (!UpdateDeviceTime(db))
            {
                return false;
            }
        }
        return StoreExtraData(db);
    }

    bool ReplaceDeviceGPS(mongocxx::database& db)
    {
        LogDebug("WioT", "Storing GPS data for device %s : lng %.3lf, lat %.3lf"
            , m_DeviceId.c_str(), m_GpsData.longitude, m_GpsData.latitude);
        mongocxx::collection devices = db["devices"];

        auto builder = bsoncxx::builder::stream::document{};
        bsoncxx::document::value gps_value = builder
            << "IMEI" << m_DeviceId
            << "time_device" << bsoncxx::types::b_date(std::chrono::milliseconds(m_GpsData.timestamp))
            << "time_server" << bsoncxx::types::b_date(std::chrono::system_clock::now())
            << "priority" << m_GpsData.priority
            << "longitude" << m_GpsData.longitude
            << "latitude" << m_GpsData.latitude
            << "altitude" << m_GpsData.altitude
            << "angle" << m_GpsData.angle
            << "satellites" << m_GpsData.satellites
            << "speed" << m_GpsData.speed
            << finalize;

        mongocxx::options::replace options;
        options.upsert(true);
        devices.replace_one(document{} << "IMEI" << m_DeviceId << finalize, gps_value.view(), options);

        return true;
    }

    bool UpdateDeviceTime(mongocxx::database& db)
    {
        LogDebug("WioT", "Updating device %s time", m_DeviceId.c_str());

        mongocxx::collection devices = db["devices"];
        auto builder = bsoncxx::builder::stream::document{};
        bsoncxx::document::value gps_value = builder
            << "$set" << open_document
            << "time_device" << bsoncxx::types::b_date(std::chrono::milliseconds(m_GpsData.timestamp))
            << "time_server" << bsoncxx::types::b_date(std::chrono::system_clock::now())
            << close_document
            << finalize;

        mongocxx::options::update options;
        options.upsert(true);
        devices.update_one(document{} << "IMEI" << m_DeviceId << finalize, gps_value.view(), options);

        return true;
    }

    bool StoreExtraData(mongocxx::database& db)
    {
        LogDebug("WioT", "Updating device %s time", m_DeviceId.c_str());
        std::ostringstream oss;
        oss << "device_" << m_DeviceId;
        mongocxx::collection device = db[oss.str().c_str()];
        auto builder = bsoncxx::builder::stream::document{};

        auto context = builder
            << "time_device" << bsoncxx::types::b_date(std::chrono::milliseconds(m_GpsData.timestamp))
            << "time_server" << bsoncxx::types::b_date(std::chrono::system_clock::now())
            << "priority" << m_GpsData.priority
            << "longitude" << m_GpsData.longitude
            << "latitude" << m_GpsData.latitude
            << "altitude" << m_GpsData.altitude
            << "angle" << m_GpsData.angle
            << "satellites" << m_GpsData.satellites
            << "speed" << m_GpsData.speed
            << "data" << open_document;

        for (const auto& data : m_ExtraData)
        {
             switch (data.Value.GetType())
            {
            case CValue::EType::UInt8:
                context << data.Name << (int32_t)data.Value.GetValue<uint8_t>();
                break;
           case CValue::EType::UInt16:
                context << data.Name << (int32_t)data.Value.GetValue<uint16_t>();
                break;
            case CValue::EType::UInt32:
                context << data.Name << (int64_t)data.Value.GetValue<uint32_t>();
                break;
            case CValue::EType::UInt64:
                context << data.Name << (int64_t)data.Value.GetValue<uint64_t>();
                break;
            case CValue::EType::Int8:
                context << data.Name << (int32_t)data.Value.GetValue<int8_t>();
                break;
            case CValue::EType::Int16:
                context << data.Name << data.Value.GetValue<int16_t>();
                break;
            case CValue::EType::Int32:
                context << data.Name << data.Value.GetValue<int32_t>();
                break;
            case CValue::EType::Int64:
                context << data.Name << data.Value.GetValue<int64_t>();
                break;
            case CValue::EType::Flt32:
                context << data.Name << data.Value.GetValue<float>();
                break;
            case CValue::EType::Flt64:
                context << data.Name << data.Value.GetValue<double>();
                break;
            case CValue::EType::String:
                context << data.Name << data.Value.GetValue<const char *>();
                break;
            default:
                break;
            }
        }

        bsoncxx::document::value gps_value = context << close_document << finalize;
        device.insert_one(gps_value.view());

        return true;
    }

private:
    std::string m_DeviceId;
    TGPSdata m_GpsData;
    CArrayData m_ExtraData;
};
