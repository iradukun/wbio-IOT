#pragma once

#include "ProtocolBase.h"
#include "duktape.h"
#include <boost/thread.hpp>
#include "boost/thread/mutex.hpp"
#include <boost/atomic.hpp>
#include <unordered_map>

class CDualCamContext;


typedef unsigned long LCamReqId;
constexpr LCamReqId InvalidCamReqId = ~LCamReqId(0);

constexpr int Type_Video = 0;
constexpr int Type_Photo = 1;
constexpr int Source_Front = 1;
constexpr int Source_Rear = 2;

class CProtocolDualCam
    : public CProtocolBase
{
private:
    CProtocolDualCam();
    ~CProtocolDualCam();

    const char* GetName() const override;

    bool Decode(CDevice* pDevice) const override;
    bool Attach(CDevice* pDevice) const  override;
    bool Detach(CDevice* pDevice) const  override;
    const TDeviceFunction* GetDeviceFunctions(int& nNbFuntions) const override;

    enum class EHandleReturn
    {
        Error = -1,
        Success,
        NotEnoughData,
    };

//    EHandleReturn HandleWelcome(CDevice* pDevice) const;
    EHandleReturn HandleMessage(CDevice* pDevice) const;

public:
    static const char* Name();

    static LCamReqId FollowCamRequest(CDevice* pDevice, int nType, int nSource, uint64_t nTimestamp, int nDuration, const char *zFilename);
    static bool GetCamRequestStatus(LCamReqId lCamReqId, std::string &status);
private:
    EHandleReturn WaitingInitialization(CDevice* pDevice, CDeviceBuffer& buffer, CDualCamContext* pContext) const;
    EHandleReturn WaitingStart(CDevice* pDevice, CDeviceBuffer& buffer, CDualCamContext* pContext) const;
    EHandleReturn WaitingData(CDevice* pDevice, CDeviceBuffer& buffer, CDualCamContext* pContext) const;
    EHandleReturn WaitingSynchronize(CDevice* pDevice, CDeviceBuffer& buffer, CDualCamContext* pContext) const;
    EHandleReturn WaitingFileMetadata(CDevice* pDevice, CDeviceBuffer& buffer, CDualCamContext* pContext) const;

#if 0
    int Decode5(CDevice* pDevice, const int nDataCount, const int nDataLength) const;

    bool SendCommand(CDevice* pDevice, const std::string& zCommand) const;

    bool OnCommand(CDevice* pDevice, const std::string& zCommand) const;
    bool OnResponse(CDevice* pDevice, const std::string& zResponse) const;
#endif

    bool RequestNextFile(CDevice* pDevice, CDualCamContext* pContext) const;
    bool SendFileRequest(CDevice* pDevice, const char* zFileIdentifier) const;
    bool SendFileTransferStatus(CDevice* pDevice, uint32_t status) const;
    bool SendQueryFileMetadataRequest(CDevice* pDevice, const char* zFileIdentifier) const;
    bool SendCloseSession(CDevice* pDevice) const;
    bool SendResume(CDevice* pDevice, uint32_t nOffset) const;
    bool SendRepeatInit(CDevice* pDevice) const;

    static uint16_t ComputeCRC(const uint8_t* pData, const int nLength, const uint16_t nInitialValue);

    static CProtocolDualCam ms_Instance;

    friend class CDualCamRequest;

public:
    static CProtocolDualCam& Instance()
    {
        return ms_Instance;
    }

    struct TCamRequest
    {
        TCamRequest();
        ~TCamRequest();

        LCamReqId lId;
        CAutoPtr<CDevice> pDevice;
        int nType;
        int nSource;
        int nTrigger;
        uint64_t nTimestamp;
        int nDuration, nFps;
        bool bFinished;
        int nProgress, nFileSize;

        char zCurrentStatus[256];
        time_t tRegisterTime;
        std::string zFilename;

        void AddRef();
        bool Release();

    private:
        boost::atomic<long> lRefCount;
    };

    static void SetCameraUrl(const std::string& zUrl);
    static const std::string& GetCameraUrl();
    static void SetCameraBasePath(const std::string& zBasePath);
    static const std::string& GetCameraBasePath();

private:
    boost::mutex m_Mutex;
    std::unordered_map<LCamReqId, CAutoPtr<TCamRequest> > m_CamRequests;
    LCamReqId m_lNextCamReqId;

    void PurgeOldRequests();

    std::string m_zCameraUrl;
    std::string m_zCameraBasePath;
};

