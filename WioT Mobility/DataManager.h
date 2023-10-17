#pragma once
#include <boost/lockfree/queue.hpp>
#include <boost/thread.hpp>
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition.hpp"
#include <vector>
#include <string>
#include <unordered_map>

#include <librdkafka/rdkafka.h>
#include "Value.h"

using namespace std;

struct TGPSdata
{
	uint64_t timestamp;
	int priority;
	// GPS data
	double longitude;
	double latitude;
	double altitude;
	double angle;
	int satellites;
	double speed;
	enum class EBatteryCharging
	{
		NotAvailable,
		Charging,
		NotCharging,
	} battery_charging;
	int battery_level; // 1 to 100; <=0 means not available
};

struct TExtraData
{
	std::string Name;
	CValue Value;
};
typedef std::vector<TExtraData> CArrayData;

class IDataMessage
{
public:
	virtual ~IDataMessage() {}
	virtual bool Process(rd_kafka_t *rk) = 0;
};


struct TVideoRecordState
{
	typedef long LRequestId;

	const char* zType;
	const char* zSource;
	const char* zTrigger;
	uint64_t Timestamp;
	int32_t Duration;
	int32_t Progress;
	LRequestId ulRequestId;
	int32_t lLength;
	std::string zState;
	std::string zUrl;
};

struct VehicleImeiData {
	int vehicleId;
	string vehicle_uuid;
};

constexpr TVideoRecordState::LRequestId NoRequestId = ~(TVideoRecordState::LRequestId{ 0 });

class CDataManager
{
private:
	CDataManager();
	~CDataManager();

public:
	bool Start();
	bool Stop();

	void SetServers(const char* zServers);
	void SetClientId(const char* zClientId);
	void SetTopicVehicle(const char* zTopic);
	void SetTopicVideo(const char* zTopic);
	void SetVehicleFileName(const char* zFileName);

	bool PushData(const std::string& DeviceId, const TGPSdata& GpsData, const CArrayData &ExtraData);
	bool PushVideo(const std::string& DeviceId, const TVideoRecordState& VideoState);

	static CDataManager& GetInstance();
private:
	boost::thread m_Thread;
	boost::thread m_ConsumerThread;
	boost::mutex m_Mutex;
	boost::mutex m_ConsumerMutex;

	boost::atomic<bool> m_bWantStop;
	std::string m_zServers;
	std::string m_zClientId;
	std::string m_zTopicVehicleData, m_zTopicVideo;
	string m_zVehicleFileName;
	std::unordered_map<string, VehicleImeiData> mapImeiVehicle;
	rd_kafka_topic_t* m_rkTopicVehicleData;
	rd_kafka_topic_t* m_rkTopicVideo;

	bool InitProducer(rd_kafka_t*& rk, rd_kafka_conf_t*& conf);
	bool InitConsumer(rd_kafka_t*& rk, rd_kafka_conf_t*& conf);
	bool InitTopic(rd_kafka_t* rk, const char *zTopicName, rd_kafka_topic_t*& topic, rd_kafka_topic_conf_t*& topic_conf);
	void Thread();
	void ConsumerThread();
	void ConsumeMessage(rd_kafka_t* rk);
	void LoadVehicleData();
	void PersistVehicleData();
	void BuildVehiclePayload(const std::string& DeviceId, const std::string& zDate, const TGPSdata& GpsData, const CArrayData& ExtraData, std::string& payload);
	static void ErrorCallback(rd_kafka_t* rk, int err, const char* reason, void* opaque);
	static void LogCallback(const rd_kafka_t* rk, int level, const char* fac, const char* buf);
};

#define DataManager CDataManager::GetInstance()

