#include "WioTMobility.h"
#include "ProtocolDualCam.h"
#include "ProtocolTeltonika.h"
#include "DataManager.h"
#include "Device.h"
#include "DeviceRequest.h"
#include "DeviceFunction.h"
#include "AutoPtr.h"
#include <arpa/inet.h>
#include <cctype>
#include <sstream>
#include <inttypes.h>
#include <sys/stat.h>
#include <utility>
#include <boost/filesystem.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include "deelx.h"
#pragma GCC diagnostic pop

#define MAX_PATH (1024)
constexpr const char* BaseVideoPath = "/tmp/";
constexpr double RequestLifeTime = 5. * 60.;

static const char* TriggerToString(const int trigger)
{
    if (trigger & 2)
    {
        return "DIN1";
    }
    if (trigger & 4)
    {
        return  "DIN2";
    }
    if (trigger & 8)
    {
        return  "Crash";
    }
    if (trigger & 16)
    {
        return  "Towing";
    }
    if (trigger & 32)
    {
        return  "Idling";
    }
    if (trigger & 64)
    {
        return  "Geofence";
    }
    if (trigger & 128)
    {
        return  "Unplug";
    }
    return nullptr;
}

static const char* GetVideoType(int nType)
{
    switch (nType)
    {
    case Type_Video:
        return "video";
    case Type_Photo:
        return "photo";
    }
    return "unknow";
}

static const char* GetVideoSource(int nSource)
{
    switch (nSource)
    {
    case Source_Front:
        return "front";
    case Source_Rear:
        return "rear";
    }
    return "unknow";
}

CProtocolDualCam::TCamRequest::TCamRequest()
    : lId(NoRequestId)
	, nType{ 0 }
	, nSource{ 0 }
	, nTrigger{ 0 }
	, nTimestamp{ 0 }
	, nDuration{ 0 }
	, bFinished{ false }
	, nProgress{ 0 }
	, nFileSize{ 0 }
    , tRegisterTime{ 0 }
	, lRefCount(1)
{
    memset(zCurrentStatus, 0, sizeof(zCurrentStatus));
}

CProtocolDualCam::TCamRequest::~TCamRequest()
{
}

void CProtocolDualCam::TCamRequest::AddRef()
{
    lRefCount++;
}

bool CProtocolDualCam::TCamRequest::Release()
{
    if (--lRefCount == 0)
    {
        delete this;
        return true;
    }
    return false;
}


static const char* const gs_zFileIdentifiers[] =
{
    "%videor", "%videof", "%photor", "%photof"
};



enum class EDualCamCommand
{
    CloseSession = 0,
    StartFileTransfer = 1,
    ResumeFileTransfer = 2,
    SynchronizeFileTransfer = 3,
    FileDataTransfer = 4,
    FileTransferStatus = 5,
    FileRequest = 8,
    RepeatInitializationPacket = 9,
    QueryFileMetadata = 10,
    FileMetadataResponse = 11,

};

class CDualCamContext : public CProtocolContext
{
public:
    CDualCamContext()
        : m_eCurrentState(EState::WaitingInitialization)
        , m_nProtocolId(0)
        , m_nSettings(0)
        , m_VideoTimestamp(0)
        , m_VideoDuration(0)
        , m_File(nullptr)
        , m_nPacketCount(0)
        , m_nCurrentPacket(0)
        , m_nPreviousCRC(0)
        , m_nFileSize(0)
    {}
    virtual ~CDualCamContext()
    {
        if (m_File)
        {
            fclose(m_File);
        }
    }

    enum class EState
    {
        WaitingInitialization,
        WaitingStart,
        WaitingFileMetadata,
        WaitingSynchronize,
        WaitingData,
        Closed,
    } m_eCurrentState{ EState::WaitingInitialization };

    int m_nProtocolId, m_nSettings;
    uint64_t m_VideoTimestamp;
    int m_VideoDuration;
    std::string m_zFilename;
    CAutoPtr<CProtocolDualCam::TCamRequest> m_CurrentCamReq;

    bool StartFileTransfer(CDevice *pDevice, uint32_t nPacketCount)
    {
        if (m_zFilename.empty())
        {
            return false;
        }
        m_File = fopen(m_zFilename.c_str(), "w+b");
        if (!m_File)
        {
            const int err = errno;
            LogErr("WioT", "[DualCam] Failed to create file \"%s\".\n"
                "Error %d : %s", m_zFilename.c_str(), err, strerror(err));
            return false;
        }
        m_nPacketCount = nPacketCount;
        m_nCurrentPacket = 0;
        m_nPreviousCRC = 0;
        m_nFileSize = 0;

        if (!m_CurrentCamReq.Empty())
        {
            strcpy(m_CurrentCamReq->zCurrentStatus, "File created - Starting to upload");
            SetUploadStatus(pDevice, "File created - Starting to upload");
        }
        return true;
    }

    bool Write(CDevice* pDevice, const void* pData, int nLength)
    {
        const auto ret = fwrite(pData, 1, nLength, m_File);
        if (ret < 0)
        {
            const int err = errno;
            LogErr("WioT", "[DualCam] Error writing %d bytes.\n"
                "Error %d : %s", nLength, err, strerror(err));
            return false;
        }
        if (ret < static_cast<size_t>(nLength))
        {
            const int err = errno;
            LogWarn("WioT", "[DualCam] Not all data have been wroten (%d < %d).\n"
                "Error %d : %s", ret, nLength, err, strerror(err));
        }
        m_nFileSize += nLength;
        m_nCurrentPacket++;

        if (!m_CurrentCamReq.Empty())
        {
            m_CurrentCamReq->nFileSize = m_nFileSize;
            m_CurrentCamReq->nProgress = (m_nCurrentPacket * 100) / m_nPacketCount;
            sprintf(m_CurrentCamReq->zCurrentStatus, "Current file transfer %d bytes (%d%%)",
                m_CurrentCamReq->nFileSize, m_CurrentCamReq->nProgress);
            SetUploadStatus(pDevice, "Uploading...");
        }
        return true;
    }

    bool SetFileOffset(uint32_t nOffset)
    {
        m_nFileSize = (nOffset - 1) * 1024;
        fseek(m_File, m_nFileSize, SEEK_SET);
        return true;
    }

    bool EndFileTransfert(CDevice* pDevice)
    {
        fclose(m_File);
        m_File = 0;

        if (!m_CurrentCamReq.Empty())
        {
            if (m_CurrentCamReq->nType == Type_Photo)
            {
                // photo => move
                if (rename(m_zFilename.c_str(), m_CurrentCamReq->zFilename.c_str()))
                {
                    const int err = errno;
                    LogErr("WioT", "[DualCam] Cannot rename file \"%s\" to \"%s\"\n"
                        "Error %d : %s", m_zFilename.c_str(), m_CurrentCamReq->zFilename.c_str(),
                        err, strerror(err));
                }
                m_CurrentCamReq->bFinished = true;
                strcpy(m_CurrentCamReq->zCurrentStatus, "Completed");
                FinalizeUpload(pDevice, m_CurrentCamReq, true, m_CurrentCamReq->zFilename.c_str());
                return true;
            }
            // video => conversion
            struct TLauncher
            {
                std::string zInputFilename;
                CAutoPtr<CProtocolDualCam::TCamRequest> pRequest;
                CDevice* pDevice;

                TLauncher(const std::string& input, CDevice *device, CProtocolDualCam::TCamRequest* p)
                    : zInputFilename(input)
                    , pRequest(p)
                    , pDevice(device)
                {}

                void operator()()
                {
                    ThreadConversion(zInputFilename, pDevice, pRequest);
                }
            } launcher(m_zFilename, pDevice, m_CurrentCamReq);

            strcpy(m_CurrentCamReq->zCurrentStatus, "Conversion in progress...");
            SetUploadStatus(pDevice, "Conversion in progress...");
            boost::thread{ launcher };
        }
        return false;
    }

    uint32_t GetPacketCount() const
    {
        return m_nPacketCount;
    }

    uint32_t GetCurrentPacket() const
    {
        return m_nCurrentPacket;
    }

    uint16_t GetPreviousCRC() const
    {
        return m_nPreviousCRC;
    }

    void SetPreviousCRC(uint16_t nNewCRC)
    {
        m_nPreviousCRC = nNewCRC;
    }
    uint32_t GetFileLength() const
    {
        return m_nFileSize;
    }

    void RazUploadStatus(CDevice* pDevice)
    {
        m_nPacketCount = m_nCurrentPacket = 0;
        m_nFileSize = 0;
        m_VideoTimestamp = 0;
        m_VideoDuration = 0;
    }

    bool SetUploadStatus(CDevice* pDevice, const std::string& zStatus)
    {
        if (!pDevice || m_CurrentCamReq.Empty())
        {
            return false;
        }

        TVideoRecordState state;

        state.zType = GetVideoType(m_CurrentCamReq->nType);
        state.zSource = GetVideoSource(m_CurrentCamReq->nSource);
        const char* zTrigger = TriggerToString(m_CurrentCamReq->nTrigger);
        state.zTrigger = zTrigger ? zTrigger : "UserReq";
        state.Timestamp = m_VideoTimestamp;
        state.Duration = m_VideoDuration;
        state.Progress = m_CurrentCamReq->nProgress;
        state.ulRequestId = m_CurrentCamReq->lId;
        state.lLength = m_CurrentCamReq->nFileSize;
        state.zState = zStatus;
        state.zUrl = "";

        const std::string zDeviceID = pDevice->GetDeviceId().substr(0, pDevice->GetDeviceId().length() - 3);
        return DataManager.PushVideo(zDeviceID, state);
    }

    bool FinalizeUpload(CDevice* pDevice, CProtocolDualCam::TCamRequest *pRequest, bool bSucceeded, const std::string& zFilenameOrError)
    {
        if (!pDevice || !pRequest)
        {
            return false;
        }

        TVideoRecordState state;

        state.zType = GetVideoType(pRequest->nType);
        state.zSource = GetVideoSource(pRequest->nSource);
        const char* zTrigger = TriggerToString(m_CurrentCamReq->nTrigger);
        state.zTrigger = zTrigger ? zTrigger : "UserReq";
        state.Timestamp = m_VideoTimestamp;
        state.Duration = m_VideoDuration;
        state.Progress = pRequest->nProgress;
        state.ulRequestId = pRequest->lId;
        state.lLength = pRequest->nFileSize;
        state.zState = bSucceeded ? "Completed" : zFilenameOrError.c_str();
        if (bSucceeded)
        {
            const std::string &zCameraUrl = CProtocolDualCam::Instance().GetCameraUrl();
            std::string zFilename = zFilenameOrError;
            if (!zCameraUrl.empty())
            {
                const std::string& zCameraBasePath = CProtocolDualCam::Instance().GetCameraBasePath();
                auto pos = zFilename.find(CProtocolDualCam::Instance().GetCameraBasePath());
                if (pos != std::string::npos)
                {
                    zFilename.replace(pos, pos + zCameraBasePath.length(), zCameraUrl);
                }
            }
            state.zUrl = zFilename;
        }
        else
        {
            state.zUrl = "";
        }

        std::string zDeviceID;
        if (pRequest->pDevice &&
            pRequest->pDevice->GetDeviceId().length() > 3)
        {
            pDevice = pRequest->pDevice;
        }
        if (pDevice->GetDeviceId().length() > 3)
        {
            zDeviceID = pDevice->GetDeviceId().substr(0, pDevice->GetDeviceId().length() - 3);
        }
        else
        {
            zDeviceID = "Unknow";
        }
        return DataManager.PushVideo(zDeviceID, state);
    }

private:
    FILE* m_File;
    uint32_t m_nPacketCount, m_nCurrentPacket;
    uint16_t m_nPreviousCRC;
    uint32_t m_nFileSize;

    static void ThreadConversion(const std::string& zInputFilename, CDevice* pDevice, CProtocolDualCam::TCamRequest* pRequest);
};

#if 0
class CDualCamRequest :
    public CDeviceRequest
{
public:
    CDualCamRequest(CDevice* pDevice, const std::string &zCommand)
        : CDeviceRequest(pDevice)
        , m_zCommand(zCommand)
    {}
    virtual ~CDualCamRequest()
    {}

    bool ProcessRequest() override
    {
        // should return true to say that the request is not ended
        return CProtocolDualCam::ms_Instance.SendCommand(GetDevice(), m_zCommand);
    }
    const std::string& GetCommand() const
    {
        return m_zCommand;
    }
private:
    std::string m_zCommand;
};
#endif

CProtocolDualCam CProtocolDualCam::ms_Instance;

CProtocolDualCam::CProtocolDualCam()
    : m_lNextCamReqId{1}
    , m_zCameraBasePath(".")
{
}

CProtocolDualCam::~CProtocolDualCam()
{
}

const char* CProtocolDualCam::GetName() const
{
    return Name();
}

bool CProtocolDualCam::Decode(CDevice* pDevice) const
{
    if (!pDevice)
    {
        return false;
    }

    CDualCamContext* pContext = static_cast<CDualCamContext*>(pDevice->GetContext());
    auto& buffer = pDevice->GetBuffer();

    for (;;)
    {
        int nTrameLength = 4; // minimum size is function code + data length = 4 bytes
        if (pContext->m_eCurrentState == CDualCamContext::EState::WaitingInitialization)
        {
            nTrameLength = 16;
        }
        else if (buffer.GetAvailableCount() >= nTrameLength)
        {
            const uint16_t dataLength = NtoH(buffer.GetValue<uint16_t>(2));
            nTrameLength = 4 + dataLength;
        }

        if (buffer.GetAvailableCount() < nTrameLength)
        {
            // not enough data
            return true;
        }

        switch (HandleMessage(pDevice))
        {
        case EHandleReturn::Error:
            return false;
        case EHandleReturn::NotEnoughData:
            return true;
        case EHandleReturn::Success:
        default:
            break;
        }
    }
    // not cool but no choice
    const_cast<CProtocolDualCam*>(this)->PurgeOldRequests();
    return true;
}

bool CProtocolDualCam::Attach(CDevice* pDevice) const
{
    if (!pDevice)
    {
        return false;
    }
    CDualCamContext* pContext = new CDualCamContext;
    pDevice->SetContext(pContext);

    return true;
}

bool CProtocolDualCam::Detach(CDevice* pDevice) const
{
    CDualCamContext* pContext = static_cast<CDualCamContext*>(pDevice->GetContext());
    if (pContext && !pContext->m_CurrentCamReq.Empty())
    {
        strcpy(pContext->m_CurrentCamReq->zCurrentStatus, "Connection lost");
        pContext->SetUploadStatus(pDevice, "Connection lost");
    }
    SendCloseSession(pDevice);

    return true;
}

const TDeviceFunction* CProtocolDualCam::GetDeviceFunctions(int& nNbFuntions) const
{
    nNbFuntions = 0;
    return nullptr;
}

CProtocolDualCam::EHandleReturn CProtocolDualCam::HandleMessage(CDevice* pDevice) const
{
    auto& buffer = pDevice->GetBuffer();
    if (buffer.GetAvailableCount() < 4)
    {
        // not enough data
        return EHandleReturn::NotEnoughData;
    }

    if (!pDevice)
    {
        return EHandleReturn::Error;
    }
    CDualCamContext* pContext = static_cast<CDualCamContext*>(pDevice->GetContext());
    if (!pContext)
    {
        return EHandleReturn::Error;
    }

    switch (pContext->m_eCurrentState)
    {
    case CDualCamContext::EState::WaitingInitialization:
        return WaitingInitialization(pDevice, buffer, pContext);
    case CDualCamContext::EState::WaitingStart:
        return WaitingStart(pDevice, buffer, pContext);
    case CDualCamContext::EState::WaitingFileMetadata:
        return WaitingFileMetadata(pDevice, buffer, pContext);
    case CDualCamContext::EState::WaitingSynchronize:
        return WaitingSynchronize(pDevice, buffer, pContext);
    case CDualCamContext::EState::WaitingData:
        return WaitingData(pDevice, buffer, pContext);
    case CDualCamContext::EState::Closed:
        return EHandleReturn::Error;
    }

	return EHandleReturn::Success;
}

template<typename IOcount_t, typename IO_t>
static void ReadIO(CArrayData& ExtraData, int& n, int& nOffset, const CDeviceBuffer& buffer)
{
    const int NbIO = NtoH(buffer.ReadValue<IOcount_t>(nOffset));

    for (int i = 0; i < NbIO; ++i)
    {
        const IOcount_t IOidx = NtoH(buffer.ReadValue<IOcount_t>(nOffset));
        const IO_t Value = NtoH(buffer.ReadValue<IO_t>(nOffset));
        char Name[64];
        sprintf(Name, "%u", IOidx);
        ExtraData[n].Name = Name;
        ExtraData[n].Value = CValue(Value);
        n++;
    }
}

CProtocolDualCam::EHandleReturn CProtocolDualCam::WaitingInitialization(CDevice* pDevice, CDeviceBuffer& buffer, CDualCamContext* pContext) const
{
    if (buffer.GetAvailableCount() < 16)
    {
        // not enough data
        return EHandleReturn::NotEnoughData;
    }

    int nOffset = 0;
    const uint16_t header = NtoH(buffer.ReadValue<uint16_t>(nOffset));
    const uint16_t protocolId = NtoH(buffer.ReadValue<uint16_t>(nOffset));

    if (header != 0)
    {
        LogErr("WioT", "[%s] Protocol error : bad header (%04x)", Name(), header);
        return EHandleReturn::Error;
    }

    if (protocolId != 5)
    {
        LogErr("WioT", "[%s] Protocol error : unrecognized protocol (%d)", Name(), protocolId);
        return EHandleReturn::Error;
    }

    const uint64_t iImei = NtoH(buffer.ReadValue<uint64_t>(nOffset));

    char IMEI[64];
    snprintf(IMEI, 64, "%" PRIu64 "_DC", iImei);

    if (pDevice->GetDeviceId().empty())
    {
        if (!pDevice->SetDeviceId(IMEI))
        {
            LogInf("WioT", "Could not set device IMEI %s", IMEI);
            return EHandleReturn::Error;
        }
        LogInf("WioT", "Connected to device %s", IMEI);
    }
    else if (pDevice->GetDeviceId() != IMEI)
    {
        LogWarn("WioT", "[%s] Received Initialization packet with different IMEI %s", pDevice->GetDeviceId().c_str(), IMEI);
    }

    const uint32_t setting = NtoH(buffer.ReadValue<uint32_t>(nOffset));
    buffer.Consume(nOffset);

    pContext->m_nProtocolId = protocolId;
    pContext->m_nSettings = setting;
    LogInf("WioT", "[%s] Settings = 0x%08x", Name(), setting);

    if (!RequestNextFile(pDevice, pContext))
    {
        LogNotice("WioT", "[%s] Nothing to download", Name());
        SendCloseSession(pDevice);
    }
    return EHandleReturn::Success;
}

#if 0
static void _mkdir(const char* dir) {
    char tmp[1024];
    char* p = NULL;
    size_t len;

    strncpy(tmp, dir, sizeof(tmp));
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/')
        {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    mkdir(tmp, S_IRWXU);
}
#endif

CProtocolDualCam::EHandleReturn CProtocolDualCam::WaitingStart(CDevice* pDevice, CDeviceBuffer& buffer, CDualCamContext* pContext) const
{
    if (buffer.GetAvailableCount() < 10)
    {
        // not enough data
        return EHandleReturn::NotEnoughData;
    }

    int nOffset = 0;
    const EDualCamCommand commandId = static_cast<EDualCamCommand>(NtoH(buffer.ReadValue<uint16_t>(nOffset)));
    const uint16_t dataLength = NtoH(buffer.ReadValue<uint16_t>(nOffset));

    if (commandId != EDualCamCommand::StartFileTransfer)
    {
        LogErr("WioT", "[%s] Waiting for a StartFileTransfer command and get command 0x%x", Name(), static_cast<uint32_t>(commandId));
        return EHandleReturn::Error;
    }

    if (dataLength != 6)
    {
        LogErr("WioT", "[%s] Protocol error : data length (%hu) should be 6 for StartFileTransfer command", Name(), dataLength);
        return EHandleReturn::Error;
    }

    const uint32_t packetCount = NtoH(buffer.ReadValue<uint32_t>(nOffset));
    const uint16_t blankField = NtoH(buffer.ReadValue<uint16_t>(nOffset));
    buffer.Consume(nOffset);

    if (blankField != 0)
    {
        LogErr("WioT", "[%s] Protocol error : Blank field is not blank (%hu) for StartFileTransfer command", Name(), blankField);
        return EHandleReturn::Error;
    }

    if (packetCount == 0)
    {
        LogNotice("WioT", "[%s] Empty file reported", Name());
        if (!RequestNextFile(pDevice, pContext))
        {
            // no more file to download
            if (!SendRepeatInit(pDevice))
            {
                return EHandleReturn::Error;
            }
            pContext->m_eCurrentState = CDualCamContext::EState::WaitingInitialization;
        }
        return EHandleReturn::Success;
    }

    if (!pContext->StartFileTransfer(pDevice, packetCount))
    {
        return EHandleReturn::Error;
    }

    if (!SendResume(pDevice, 1))
    {
        return EHandleReturn::Error;
    }

    pContext->m_eCurrentState = CDualCamContext::EState::WaitingSynchronize;
    return EHandleReturn::Success;
}

CProtocolDualCam::EHandleReturn CProtocolDualCam::WaitingData(CDevice* pDevice, CDeviceBuffer& buffer, CDualCamContext* pContext) const
{
    if (buffer.GetAvailableCount() < 4)
    {
        // not enough data
        return EHandleReturn::NotEnoughData;
    }

    int nOffset = 0;
    const EDualCamCommand commandId = static_cast<EDualCamCommand>(NtoH(buffer.ReadValue<uint16_t>(nOffset)));
    const uint16_t dataLength = NtoH(buffer.ReadValue<uint16_t>(nOffset));

    if (buffer.GetAvailableCount() < 4 + dataLength)
    {
        // not enough data
        return EHandleReturn::NotEnoughData;
    }

    if (commandId == EDualCamCommand::SynchronizeFileTransfer)
    {
        const uint32_t fileOffset = NtoH(buffer.ReadValue<uint32_t>(nOffset));
        buffer.Consume(nOffset);

        return pContext->SetFileOffset(fileOffset) ? EHandleReturn::Success : EHandleReturn::Error;
    }

    if (commandId == EDualCamCommand::FileDataTransfer)
    {
        const uint16_t computedCRC = ComputeCRC(buffer.Buffer() + nOffset, dataLength - 2, pContext->GetPreviousCRC());
        const uint16_t expectedCRC = NtoH(buffer.GetValue<uint16_t>(nOffset + dataLength - 2));

        if (computedCRC != expectedCRC)
        {
#if 0
            char filename[128];
            sprintf(filename, "raw_%d.hex", pContext->GetCurrentPacket());
            FILE* file = fopen(filename, "wt");
            if (file)
            {   
                fwrite(buffer.Buffer(), 1, nOffset + dataLength, file);
                fclose(file);
            }
#endif
            buffer.Consume(nOffset + dataLength);
            LogNotice("WioT", "[%s] Bad CRC (expected 0x%04x, computed 0x%04x)", Name(), expectedCRC, computedCRC);
            if (!SendResume(pDevice, pContext->GetCurrentPacket()))
            {
                return EHandleReturn::Error;
            }
            pContext->m_eCurrentState = CDualCamContext::EState::WaitingSynchronize;
            return EHandleReturn::Success;
        }
        // write data to file
        pContext->Write(pDevice, buffer.Buffer() + nOffset, dataLength - 2);

        buffer.Consume(nOffset + dataLength);
        pContext->SetPreviousCRC(expectedCRC);

        if (pContext->GetCurrentPacket() >= pContext->GetPacketCount())
        {
            pContext->EndFileTransfert(pDevice);

            LogNotice("WioT", "[%s] File transfert ended (%d bytes).",
                pDevice->GetDeviceId().c_str(), pContext->GetFileLength());

            if (!RequestNextFile(pDevice, pContext))
            {
                // no more file to download
                if (!SendRepeatInit(pDevice))
                {
                    return EHandleReturn::Error;
                }
                pContext->m_eCurrentState = CDualCamContext::EState::WaitingInitialization;
            }
        }

        return EHandleReturn::Success;
    }

    buffer.Consume(nOffset + dataLength);

    LogErr("WioT", "[%s] Waiting for a SynchronizeFileTransfer or FileDataTransfer commands and get command 0x%x", Name(), static_cast<uint32_t>(commandId));
    return EHandleReturn::Success;
}

CProtocolDualCam::EHandleReturn CProtocolDualCam::WaitingSynchronize(CDevice* pDevice, CDeviceBuffer& buffer, CDualCamContext* pContext) const
{
    if (buffer.GetAvailableCount() < 4)
    {
        // not enough data
        return EHandleReturn::NotEnoughData;
    }

    int nOffset = 0;
    const EDualCamCommand commandId = static_cast<EDualCamCommand>(NtoH(buffer.ReadValue<uint16_t>(nOffset)));
    const uint16_t dataLength = NtoH(buffer.ReadValue<uint16_t>(nOffset));

    if (buffer.GetAvailableCount() < 4 + dataLength)
    {
        // not enough data
        return EHandleReturn::NotEnoughData;
    }

    if (commandId == EDualCamCommand::SynchronizeFileTransfer)
    {
        const uint32_t fileOffset = NtoH(buffer.ReadValue<uint32_t>(nOffset));
        buffer.Consume(nOffset);

        if (!pContext->SetFileOffset(fileOffset))
        {
            return EHandleReturn::Error;
        }
        pContext->m_eCurrentState = CDualCamContext::EState::WaitingData;
        return EHandleReturn::Success;
    }

    if (commandId == EDualCamCommand::FileDataTransfer)
    {
        // data are dropped
        buffer.Consume(nOffset + dataLength);
        return EHandleReturn::Success;
    }

    buffer.Consume(nOffset + dataLength);

    LogErr("WioT", "[%s] Waiting for a SynchronizeFileTransfer commands and get command 0x%x", Name(), static_cast<uint32_t>(commandId));
    return EHandleReturn::Success;
}

CProtocolDualCam::EHandleReturn CProtocolDualCam::WaitingFileMetadata(CDevice* pDevice, CDeviceBuffer& buffer, CDualCamContext* pContext) const
{
    int nOffset = 0;
    const EDualCamCommand commandId = static_cast<EDualCamCommand>(NtoH(buffer.ReadValue<uint16_t>(nOffset)));
    const uint16_t dataLength = NtoH(buffer.ReadValue<uint16_t>(nOffset));

    if (commandId == EDualCamCommand::FileTransferStatus)
    {
        const uint32_t status = NtoH(buffer.ReadValue<uint32_t>(nOffset));
        buffer.Consume(nOffset);

        LogErr("WioT", "[%s] Waiting for a FileMetadataResponse command and get FileTransferStatus with status %u", Name(), status);
        return SendCloseSession(pDevice) ? EHandleReturn::Success : EHandleReturn::Error;
    }

    if (commandId != EDualCamCommand::FileMetadataResponse)
    {
        LogErr("WioT", "[%s] Waiting for a FileMetadataResponse command and get command 0x%x", Name(), static_cast<uint32_t>(commandId));
        return EHandleReturn::Error;
    }

    if (dataLength != 13)
    {
        LogErr("WioT", "[%s] Protocol error : data length (%hu) should be 13 for FileMetadataResponse command", Name(), dataLength);
        return EHandleReturn::Error;
    }

    const uint8_t commandVersion = buffer.ReadValue<uint8_t>(nOffset);
    const uint8_t fileType = buffer.ReadValue<uint8_t>(nOffset);
    const uint64_t timestamp = NtoH(buffer.ReadValue<uint64_t>(nOffset));
    const uint8_t trigger = buffer.ReadValue<uint8_t>(nOffset);
    const uint16_t duration = NtoH(buffer.ReadValue<uint16_t>(nOffset));
    uint16_t fps = 30;
    buffer.Consume(nOffset);
    if (buffer.GetAvailableCount() == 1)
    {
        LogDebug("WioT", "[%s] Left one byte in the buffer after FileMetadataResponse (0x%02x). Skip it.", Name(), buffer.GetValue<uint8_t>(0));
        fps = buffer.GetValue<uint8_t>(0);
        buffer.Consume(1);
    }
    const char* zFileIdentifier = nullptr;
    const char* zFilename = nullptr;
    const char* zExt = nullptr;

    int nType, nSource;
    if (fileType & 0x20)
    {
        zFileIdentifier = gs_zFileIdentifiers[0];
        zFilename = "video_rear";
        zExt = "h265";
        nType = Type_Video;
        nSource = Source_Rear;
    }
    else if (fileType & 0x10)
    {
        zFileIdentifier = gs_zFileIdentifiers[1];
        zFilename = "video_front";
        zExt = "h265";
        nType = Type_Video;
        nSource = Source_Front;
    }
    else if (fileType & 0x8)
    {
        zFileIdentifier = gs_zFileIdentifiers[2];
        zFilename = "photo_rear";
        zExt = "jpg";
        nType = Type_Photo;
        nSource = Source_Rear;
   }
    else if (fileType & 0x4)
    {
        zFileIdentifier = gs_zFileIdentifiers[3];
        zFilename = "photo_front";
        zExt = "jpg";
        nType = Type_Photo;
        nSource = Source_Front;
    }
    else
    {
        LogDebug("WioT", "[%s] Metadata received but no file is specified",
            pDevice->GetDeviceId().c_str());
        return EHandleReturn::Error;
    }

    LogDebug("WioT", "[%s] Metadata of file %s : cmdVer=%u, filetype=%u, timestamp=%" PRIu64 ", trigger=%u, duration=%hu",
        pDevice->GetDeviceId().c_str(), zFileIdentifier, commandVersion, fileType, timestamp, trigger, duration);

    if (timestamp == 0)
    {
        if (!RequestNextFile(pDevice, pContext))
        {
            LogNotice("WioT", "[%s] Nothing to download", Name());
            SendFileTransferStatus(pDevice, 0);
        }
        return EHandleReturn::Success;
    }

    pContext->m_CurrentCamReq = nullptr;
    {
        const std::string& zDeviceId = pDevice->GetDeviceId();
        const auto len = zDeviceId.length() - 3;
        boost::mutex::scoped_lock lock(ms_Instance.m_Mutex);
        for (auto& req : m_CamRequests)
        {
            if (!req.second->bFinished &&
                zDeviceId.compare(0, len, req.second->pDevice->GetDeviceId()) == 0 &&
                req.second->nType == nType &&
                req.second->nSource == nSource)
            {
                if (nType == Type_Photo ||
                    (req.second->nTimestamp * 1000 == timestamp &&
                    req.second->nDuration == duration))
                {
                    pContext->m_CurrentCamReq = req.second;
                    pContext->m_CurrentCamReq->nTrigger = trigger;
                    strcpy(req.second->zCurrentStatus, "Connected to device");
                    pContext->SetUploadStatus(pDevice, "Connected to device");
                    break;
                }
            }
        }

        if (pContext->m_CurrentCamReq.Empty())
        {
            // a video or a photo is comming not associated to a request
            // we create a fake request based on information in order for
            // the file to be copied the right way
            CAutoPtr<TCamRequest> request(new TCamRequest, false);

            request->pDevice = pDevice;
            request->nType = nType;
            request->nSource = nSource;
            request->nTrigger = trigger;
            request->nTimestamp = timestamp / 1000;
            request->nDuration = duration;
            request->nFps = fps;

            std::ostringstream oss;

            if (!m_zCameraBasePath.empty())
            {
                oss << m_zCameraBasePath << "/";
            }
            const char* zTrigger = TriggerToString(trigger);

            const std::string zDeviceID = pDevice->GetDeviceId().substr(0, pDevice->GetDeviceId().length() - 3);
            if (zTrigger)
            {
                const time_t t = static_cast<time_t>(timestamp / 1000);
                struct tm* tm = gmtime(&t);
                oss << zFilename << "_"
                    << (tm->tm_year + 1900) << "_" << (tm->tm_mon + 1) << "_" << tm->tm_mday << "_"
                    << zTrigger << "_" << zDeviceID << "_";

                for (int i = 0; i < 1000000; ++i)
                {
                    char ext[64];
                    snprintf(ext, 64, (nType == Type_Video) ? "%d.mp4" : "%d.jpg", i);
                    request->zFilename = oss.str() + ext;
                    if (!boost::filesystem::exists(request->zFilename))
                    {
                        break;
                    }
                }
            }
            else
            {
                oss << zFilename << "_" << (timestamp / 1000) << "_" << duration << "_" << zDeviceID << ((nType == Type_Video) ? ".mp4" : ".jpg");
                request->zFilename = oss.str();
            }

            strcpy(request->zCurrentStatus, "Connected to device");
            pContext->SetUploadStatus(pDevice, "Connected to device");

            request->tRegisterTime = time(nullptr);

            pContext->m_CurrentCamReq = request;

            LogInf("WioT", "[%s] Creating a fake request that will send data to %s",
                pDevice->GetDeviceId().c_str(), request->zFilename.c_str());
        }
    }

    if (!SendFileRequest(pDevice, zFileIdentifier))
    {
        return EHandleReturn::Error;
    }

    char zFullPath[MAX_PATH];
    snprintf(zFullPath, CountOf(zFullPath), "%s/%s_%" PRIu64 "_%d_%s.%s",
        BaseVideoPath,
        zFilename, timestamp, duration, pDevice->GetDeviceId().c_str(), zExt);

    pContext->m_zFilename = zFullPath;
    pContext->m_VideoTimestamp = timestamp;
    pContext->m_VideoDuration = duration;
    pContext->m_eCurrentState = CDualCamContext::EState::WaitingStart;

    return EHandleReturn::Success;
}

#if 0
int CProtocolDualCam::Decode5(CDevice* pDevice, const int nDataCount, const int nDataLength) const
{

    const auto& buffer = pDevice->GetBuffer();
    constexpr int StartOffset = 10;
    int nOffset = StartOffset;
    for (int i = 0; i < nDataCount; ++i)
    {
        TGPSdata GpsData;
        GpsData.timestamp = NtoH(buffer.ReadValue<uint64_t>(nOffset));

        GpsData.priority = NtoH(buffer.ReadValue<uint8_t>(nOffset));
        // GPS GpsData
        GpsData.longitude = NtoH(buffer.ReadValue<uint32_t>(nOffset)) * 1.e-7;
        GpsData.latitude = NtoH(buffer.ReadValue<uint32_t>(nOffset)) * 1.e-7;
        GpsData.altitude = NtoH(buffer.ReadValue<uint16_t>(nOffset));
        GpsData.angle = NtoH(buffer.ReadValue<uint16_t>(nOffset));
        GpsData.satellites = NtoH(buffer.ReadValue<uint8_t>(nOffset));
        GpsData.speed = NtoH(buffer.ReadValue<uint16_t>(nOffset));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
        // Event IO
        const IOcount_t eventIOID = NtoH(buffer.ReadValue<IOcount_t>(nOffset));
        const IOcount_t NbTotalIO = NtoH(buffer.ReadValue<IOcount_t>(nOffset));

        CArrayData ExtraData(NbTotalIO);
        int n = 0;
        ReadIO<IOcount_t, uint8_t>(ExtraData, n, nOffset, buffer);

        // We do this here since battery level is 8 bits width.
        // So we will search in a more little ensemble
        GpsData.battery_charging = TGPSdata::EBatteryCharging::NotAvailable;
        GpsData.battery_level = -1;
        for (const auto& data : ExtraData)
        {
            if (data.Name == "113" &&
                data.Value.GetType() == CValue::EType::UInt8)
            {
                GpsData.battery_level = data.Value.GetValue<uint8_t>();
                break;
            }
        }

        ReadIO<IOcount_t, uint16_t>(ExtraData, n, nOffset, buffer);
        ReadIO<IOcount_t, uint32_t>(ExtraData, n, nOffset, buffer);
        ReadIO<IOcount_t, uint64_t>(ExtraData, n, nOffset, buffer);

        if (IncludeVariableLengthIO)
        {
            const IOcount_t NbXByteIO = NtoH(buffer.ReadValue<IOcount_t>(nOffset));

            for (IOcount_t i = 0; i < NbXByteIO; ++i)
            {
                const IOcount_t NXthIOid = NtoH(buffer.ReadValue<IOcount_t>(nOffset));
                const IOcount_t NXthLength = NtoH(buffer.ReadValue<IOcount_t>(nOffset));

                // pass
                char Name[64];
                sprintf(Name, "%u", NXthIOid);
                ExtraData[n].Name = Name;

                const uint8_t* pBuffer = buffer.Buffer() + nOffset;
                bool bIsAscii = true;
                for (IOcount_t i = 0; i < NXthLength; ++i)
                {
                    if (!isprint(pBuffer[i]))
                    {
                        bIsAscii = false;
                        break;
                    }
                }
                std::string Value;
                if (bIsAscii)
                {
                    Value = std::string(reinterpret_cast<const char*>(buffer.Buffer() + nOffset), NXthLength);
                }
                else
                {
                    std::ostringstream oss;
                    oss << "0x";
                    for (IOcount_t i = 0; i < NXthLength; ++i)
                    {
                        oss << std::setfill('0') << std::setw(2) << std::right << std::hex << (unsigned)pBuffer[i];
                    }
                    Value = oss.str();
                }
                ExtraData[n].Value = CValue(Value);
                n++;
                nOffset += NXthLength;
            }
        }

#pragma GCC diagnostic pop

        if (nOffset - StartOffset > nDataLength)
        {
            LogErr("WioT", "[%s] Error while decoding. Not enough GpsData.", Name());
            break;
        }

        DataManager.PushData(pDevice->GetDeviceId(), GpsData, ExtraData);
    }

    uint32_t response = NtoH(static_cast<uint32_t>(nDataCount));
    pDevice->Send(&response, sizeof(response));

    return nDataCount;
}


bool CProtocolDualCam::SendCommand(CDevice* pDevice, const std::string& zCommand) const
{
    CBuffer<512> buffer;

    buffer.AppendValue<uint32_t>(0);
    buffer.AppendValue<uint32_t>(0);
    buffer.AppendValue<uint8_t>(0x0c);
    buffer.AppendValue<uint8_t>(0x01);
    buffer.AppendValue<uint8_t>(0x05);

    const uint32_t nCmdLength = static_cast<uint32_t>(zCommand.length());
    buffer.AppendValue<uint32_t>(HtoN(nCmdLength));
    buffer.Append(zCommand.c_str(), nCmdLength);
    buffer.AppendValue<uint8_t>(0x01);

    const uint32_t nMsgLength = buffer.GetAvailableCount() - 8;
    const uint32_t CRC = ComputeCRC(buffer.Buffer() + 8, nMsgLength);
    buffer.AppendValue<uint32_t>(HtoN(CRC));
    buffer.SetValue<uint32_t>(HtoN(nMsgLength), 4);

    return pDevice->Send(buffer.Buffer(), buffer.GetAvailableCount());
}

bool CProtocolDualCam::OnCommand(CDevice* pDevice, const std::string& zCommand) const
{
    // for the moment, no command are supported
    return false;
}

static bool ParseTuples(const std::string& zResponse, std::string &json)
{

    CRegexpA regex(" *([^:]*):([^ ]*)");
    MatchResult result = regex.Match(zResponse.c_str());

    if (!result.IsMatched())
    {
        return false;
    }

    std::ostringstream oss;
    oss << "{";
    bool bFirst = true;
    do
    {
        if (!bFirst)
        {
            oss << ",";
        }
        else
        {
            bFirst = false;
        }
        oss << "\"" << zResponse.substr(result.GetGroupStart(1), result.GetGroupEnd(1) - result.GetGroupStart(1))
            << "\":\"" << zResponse.substr(result.GetGroupStart(2), result.GetGroupEnd(2) - result.GetGroupStart(2)) << "\"";

        // get next
        result = regex.Match(zResponse.c_str(), result.GetEnd());
    } while (result.IsMatched());

    oss << "}";
    json = oss.str();

    return true;
}

static bool ParseArrayValues(const std::string& zResponse, std::string &json, const char *zPattern)
{
    CRegexpA regexParam(zPattern);
    MatchResult result = regexParam.Match(zResponse.c_str());
    if (!result.IsMatched())
    {
        return false;
    }

    std::string zID = zResponse.substr(result.GetGroupStart(1), result.GetGroupEnd(1) - result.GetGroupStart(1));
    std::string zValue = zResponse.substr(result.GetGroupStart(2), result.GetGroupEnd(2) - result.GetGroupStart(2));

    std::ostringstream oss;

    CRegexpA regexValue(";(\\d+):([^;]*)");
    result = regexValue.Match(zValue.c_str());
    if (!result.IsMatched())
    {
        // only one value
        oss << "{\"" << zID << "\":\"" << zValue << "\"}";
    }
    else
    {
        oss << "{\"" << zID << "\":\"" << zValue.substr(0, result.GetStart()) << "\"";
        do
        {
            oss << ",\"" << zValue.substr(result.GetGroupStart(1), result.GetGroupEnd(1) - result.GetGroupStart(1))
                << "\":\"" << zValue.substr(result.GetGroupStart(2), result.GetGroupEnd(2) - result.GetGroupStart(2)) << "\"";

            // get next
            result = regexValue.Match(zValue.c_str(), result.GetEnd());
        } while (result.IsMatched());

        oss << "}";
    }
    json = oss.str();

    return true;
}

static bool ParseIdValues(const std::string& zResponse, std::string &json)
{
    return ParseArrayValues(zResponse, json, "(?>Param ID|IO ID):(\\d+) Value:(.*)");
}

static bool ParseNewValues(const std::string& zResponse, std::string &json)
{
    return ParseArrayValues(zResponse, json, "New value (\\d+):(.*)");
}


bool CProtocolDualCam::OnResponse(CDevice* pDevice, const std::string& zResponse) const
{
    CAutoPtr<CDeviceRequest> pRequest = pDevice->GetCurrentRequest();

    if (pRequest.Empty())
    { 
        return true;
    }
    CDualCamRequest* pTeltReq = static_cast<CDualCamRequest*>(static_cast<CDeviceRequest*>(pRequest));

    if (pTeltReq->GetCommand() == "getver")
    {
        CRegexpA regex("Hw:([^ ]*)");

        MatchResult result;
        result = regex.Match(zResponse.c_str());
        if (result.IsMatched())
        {
            std::string zType = "DualCam " + zResponse.substr(result.GetGroupStart(1), result.GetGroupEnd(1) - result.GetGroupStart(1));
            pDevice->SetDeviceType(zType);
        }
    }

    std::string json;
    bool bRet;
    
    if (zResponse.compare(0, 9, "Param ID:") == 0 ||
        zResponse.compare(0, 6, "IO ID:") == 0)
    {
        bRet = ParseIdValues(zResponse, json);
    }
    else if (zResponse.compare(0, 10, "New value ") == 0)
    {
        bRet = ParseNewValues(zResponse, json);
    }
    else
    {
        bRet = ParseTuples(zResponse, json);
    }

    if (bRet)
    {
        pRequest->ProcessSucceeded(json);
    }
    else
    {
        pRequest->ProcessFailed(zResponse);
    }

    // for the moment, no response are waited
    return true;
}
#endif

bool CProtocolDualCam::RequestNextFile(CDevice* pDevice, CDualCamContext* pContext) const
{
    const char* zFilename = nullptr;
    int mask = 0x20000000;
    for (int i = 0; i < CountOf(gs_zFileIdentifiers); ++i)
    {
        if (pContext->m_nSettings & mask)
        {
            zFilename = gs_zFileIdentifiers[i];
            pContext->m_nSettings &= ~mask;
            break;
        }
        mask >>= 1;
    }
    if (!zFilename)
    {
        return false;
    }

    if (!SendQueryFileMetadataRequest(pDevice, zFilename))
    {
        return false;
    }

    pContext->m_eCurrentState = CDualCamContext::EState::WaitingFileMetadata;
    return true;
}

bool CProtocolDualCam::SendFileRequest(CDevice *pDevice, const char* zFileIdentifier) const
{
    CBuffer<64> Cmd;

    Cmd.AppendValue<uint16_t>(HtoN(static_cast<uint16_t>(EDualCamCommand::FileRequest)));
    const uint16_t length = static_cast<uint16_t>(strlen(zFileIdentifier));
    Cmd.AppendValue<uint16_t>(HtoN(length));
    Cmd.AppendData(zFileIdentifier, length);

    return pDevice->Send(Cmd.Buffer(), Cmd.GetAvailableCount());
}

bool CProtocolDualCam::SendFileTransferStatus(CDevice* pDevice, uint32_t status) const
{
    CBuffer<8> Cmd;

    Cmd.AppendValue<uint16_t>(HtoN(static_cast<uint16_t>(EDualCamCommand::FileTransferStatus)));
    Cmd.AppendValue<uint16_t>(4);
    Cmd.AppendValue<uint32_t>(status);

    return pDevice->Send(Cmd.Buffer(), Cmd.GetAvailableCount());
}

bool CProtocolDualCam::SendQueryFileMetadataRequest(CDevice* pDevice, const char* zFileIdentifier) const
{
    CBuffer<64> Cmd;

    Cmd.AppendValue<uint16_t>(HtoN(static_cast<uint16_t>(EDualCamCommand::QueryFileMetadata)));
    const uint16_t length = static_cast<uint16_t>(strlen(zFileIdentifier));
    Cmd.AppendValue<uint16_t>(HtoN(length));
    Cmd.AppendData(zFileIdentifier, length);

    return pDevice->Send(Cmd.Buffer(), Cmd.GetAvailableCount());
}

bool CProtocolDualCam::SendCloseSession(CDevice *pDevice) const
{
    CBuffer<4> Cmd;

    Cmd.AppendValue<uint16_t>(HtoN(static_cast<uint16_t>(EDualCamCommand::CloseSession)));
    Cmd.AppendValue<uint16_t>(0);

    return pDevice->Send(Cmd.Buffer(), Cmd.GetAvailableCount());
}

bool CProtocolDualCam::SendResume(CDevice* pDevice, uint32_t nOffset) const
{
    CBuffer<8> Cmd;

    Cmd.AppendValue<uint16_t>(HtoN(static_cast<uint16_t>(EDualCamCommand::ResumeFileTransfer)));
    Cmd.AppendValue<uint16_t>(HtoN(uint16_t(4)));
    Cmd.AppendValue<uint32_t>(HtoN(nOffset));

    return pDevice->Send(Cmd.Buffer(), Cmd.GetAvailableCount());
}

bool CProtocolDualCam::SendRepeatInit(CDevice* pDevice) const
{
    CBuffer<4> Cmd;

    Cmd.AppendValue<uint16_t>(HtoN(static_cast<uint16_t>(EDualCamCommand::RepeatInitializationPacket)));
    Cmd.AppendValue<uint16_t>(0);

    return pDevice->Send(Cmd.Buffer(), Cmd.GetAvailableCount());
}

uint16_t CProtocolDualCam::ComputeCRC(const uint8_t* pData, const int nLength, const uint16_t nInitialValue)
{
    unsigned CRC = nInitialValue;
    constexpr unsigned Polynomial = 0x8408;

    for (int i = 0; i < nLength; ++i)
    {
        CRC ^= pData[i];

        for (int j = 0; j < 8; ++j)
        {
            bool Carry = (CRC & 1) != 0;
            CRC >>= 1;
            if (Carry)
            {
                CRC ^= Polynomial;
            }
        }
    }

    return static_cast<uint16_t>(CRC);
}

void CDualCamContext::ThreadConversion(const std::string& zInputFilename, CDevice *pDevice, CProtocolDualCam::TCamRequest* pRequest)
{
    std::ostringstream oss;

    int fps = pRequest->nFps;
    if (fps < 20 || fps>30)
    {
        constexpr int default_fps = 30;
        fps = default_fps;
    }
    oss << "/usr/bin/ffmpeg -r " << fps << " -i "<< zInputFilename /* << " -ss 00:00:0.9"*/ << " -c:v libx264 " << pRequest->zFilename;
  
    LogInf("WioT", oss.str().c_str());
    if (system(oss.str().c_str()) == 0)
    {
        pRequest->bFinished = true;
        strcpy(pRequest->zCurrentStatus, "Completed");
        LogInf("WioT", "Conversion completed");
        unlink(zInputFilename.c_str());
        CDualCamContext* pContext = static_cast<CDualCamContext*>(pDevice->GetContext());
        if (pContext)
        {
            pContext->FinalizeUpload(pDevice, pRequest, true, pRequest->zFilename);
        }
    }
    else
    {
        pRequest->bFinished = true;
        strcpy(pRequest->zCurrentStatus, "Error: Conversion failed");
        LogInf("WioT", "Conversion failed");
        CDualCamContext* pContext = static_cast<CDualCamContext*>(pDevice->GetContext());
        if (pContext)
        {
            pContext->FinalizeUpload(pDevice, pRequest, false, "Conversion failed");
        }
    }
}

void CProtocolDualCam::PurgeOldRequests()
{
    const time_t now = time(nullptr);

    boost::mutex::scoped_lock lock(ms_Instance.m_Mutex);

    for (auto iter = m_CamRequests.begin(); iter != m_CamRequests.end();)
    {
        if (difftime(now, iter->second->tRegisterTime) > RequestLifeTime)
        {
            iter = m_CamRequests.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

const char* CProtocolDualCam::Name()
{
    return "DualCam";
}

LCamReqId CProtocolDualCam::FollowCamRequest(CDevice* pDevice, int nType, int nSource, uint64_t nTimestamp, int nDuration, const char* zFilename)
{
    if (!pDevice)
    {
        return InvalidCamReqId;
    }

    CAutoPtr<TCamRequest> request(new TCamRequest, false);

    request->pDevice = pDevice;
    request->nType = nType;
    request->nSource = nSource;
    request->nTimestamp = nTimestamp;
    request->nDuration = nDuration;
    if (zFilename)
    {
        request->zFilename = zFilename;
    }

    strcpy(request->zCurrentStatus, "Waiting for device connection");
    request->tRegisterTime=time(nullptr);

    ms_Instance.PurgeOldRequests();

    boost::mutex::scoped_lock lock(ms_Instance.m_Mutex);
    request->lId = ms_Instance.m_lNextCamReqId++;
    if (ms_Instance.m_lNextCamReqId == InvalidCamReqId)
    {
        ms_Instance.m_lNextCamReqId = 1;
    }
    ms_Instance.m_CamRequests.emplace(std::make_pair(request->lId, request));

    return request->lId;
}

bool CProtocolDualCam::GetCamRequestStatus(LCamReqId lCamReqId, std::string& status)
{
    ms_Instance.PurgeOldRequests();

    {
        boost::mutex::scoped_lock lock(ms_Instance.m_Mutex);

        auto iter = ms_Instance.m_CamRequests.find(lCamReqId);
        if (iter != ms_Instance.m_CamRequests.end())
        {
            status = std::string("{\"Status\":\"") + iter->second->zCurrentStatus + "\"";
            if (strcmp(iter->second->zCurrentStatus, "Completed") == 0)
            {
                status += std::string(",\"Source\":\"") + iter->second->zFilename + "\"}";
            }
            else
            {
                status += "}";
            }
            return true;
        }
    }
    status = "Request not found (finished or erased)";
    return false;
}

void CProtocolDualCam::SetCameraBasePath(const std::string& zBasePath)
{
    ms_Instance.m_zCameraBasePath = zBasePath;
}

const std::string& CProtocolDualCam::GetCameraBasePath()
{
    return ms_Instance.m_zCameraBasePath;
}

void CProtocolDualCam::SetCameraUrl(const std::string& zUrl)
{
    ms_Instance.m_zCameraUrl = zUrl;
}

const std::string& CProtocolDualCam::GetCameraUrl()
{
    return ms_Instance.m_zCameraUrl;
}
