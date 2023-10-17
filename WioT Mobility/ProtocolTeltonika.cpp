#include "WioTMobility.h"
#include "ProtocolTeltonika.h"
#include "ProtocolDualCam.h"
#include "DataManager.h"
#include "Device.h"
#include "DeviceRequest.h"
#include "DeviceFunction.h"
#include "AutoPtr.h"
#include <arpa/inet.h>
#include <cctype>
#include <sstream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include "deelx.h"
#pragma GCC diagnostic pop

class CTeltonikaRequest :
    public CDeviceRequest
{
public:
    CTeltonikaRequest(CDevice* pDevice, const std::string &zCommand)
        : CDeviceRequest(pDevice)
        , m_zCommand(zCommand)
    {
        LogDebug("WioT", "Creating request for sending command \"%s\" to device %s.",
            zCommand.c_str(), pDevice->GetDeviceId().c_str());
    }
    virtual ~CTeltonikaRequest()
    {
        LogDebug("WioT", "Deleting \"%s\" command request to device %s.",
            m_zCommand.c_str(), GetDevice()->GetDeviceId().c_str());
    }

    bool ProcessRequest() override
    {
        LogDebug("WioT", "Processing \"%s\" command request to device %s.",
            m_zCommand.c_str(), GetDevice()->GetDeviceId().c_str());
        // should return true to say that the request is not ended
        return CProtocolTeltonika::ms_Instance.SendCommand(GetDevice(), m_zCommand);
    }
    const std::string& GetCommand() const
    {
        return m_zCommand;
    }
private:
    std::string m_zCommand;
};


CProtocolTeltonika CProtocolTeltonika::ms_Instance;

CProtocolTeltonika::CProtocolTeltonika()
    : m_nCameraRequestServerPort(0)
{
}

CProtocolTeltonika::~CProtocolTeltonika()
{
}

const char* CProtocolTeltonika::GetName() const
{
    return Name();
}

bool CProtocolTeltonika::Decode(CDevice* pDevice) const
{
    auto& buffer = pDevice->GetBuffer();

    for (;;)
    {
        if (buffer.GetAvailableCount() < 4)
        {
            // not enough data
            return true;
        }

        const uint32_t Preamble = NtoH(buffer.GetValue<uint32_t>(0));
        if (Preamble != 0)
        {
            switch (HandleWelcome(pDevice))
            {
            case EHandleReturn::Error:
                return false;
            case EHandleReturn::NotEnoughData:
                return true;
            case EHandleReturn::Success:
            default:
                break;
            }
            continue;
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
    return true;
}

bool CProtocolTeltonika::Attach(CDevice* pDevice) const
{
    return true;
}

bool CProtocolTeltonika::Detach(CDevice* pDevice) const
{
    return true;
}

const TDeviceFunction* CProtocolTeltonika::GetDeviceFunctions(int& nNbFuntions) const
{
    static const TDeviceFunction s_Functions[] =
    {
        // common commands
        { "sendCommand", 1, CALLABLE_1(CProtocolTeltonika, FnSendCommand, std::string) },
        { "getver", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "getinfo", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "getstatus", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "getgps", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "getio", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "ggps", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "readio", 1, CALLABLE_1(CProtocolTeltonika, FnCommand1<int>, int) },
        { "fwstats", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "getimeiccid", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "getimsi", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "allver", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "getparam", 1, CProtocolTeltonika::FnGetParam },
        { "setparam", DUK_VARARGS, CProtocolTeltonika::FnSetParam },
        { "countrecs", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "deleterecords", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "battery", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "wdlog", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "bbread", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "bbinfo", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        // bluetooth commands
        { "btgetlist", 1, CALLABLE_1(CProtocolTeltonika, FnCommand1<int>, int) },
        { "btscan", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "btvisible", 1, CALLABLE_1(CProtocolTeltonika, FnCommand1<int>, int) },
        { "btrelease", 1, CALLABLE_1(CProtocolTeltonika, FnCommand1<int>, int) },
        { "btunpair", 1, CALLABLE_1(CProtocolTeltonika, FnCommand1<const std::string &>, std::string) },
        // commands related to features
        { "fc_reset", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "towingreact", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "auto_calibrate:get", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "odoget", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "on_demand_tracking0", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "on_demand_tracking1", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "on_demand_tracking2", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        // OBD commands
        { "obdinfo", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "faultcodes", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "cleardtc", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "getvin", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        // CAN adapter commands
        { "lvcangetinfo", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "lvcansimpletacho", 1, CALLABLE_1(CProtocolTeltonika, FnCommand1<int>, int) },
        { "lvcanclear", 1, CALLABLE_1(CProtocolTeltonika, FnCommand1<int>, int) },
        { "allcanmode", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "lvcanmode", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "lvcandefaultcodes", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "lvcanopenalldoors", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "lvcanclosealldoors", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "lvcanopentrunk", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "lvcanblockengine", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "lvcanunblockengine", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "lvcanturninglights", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "lvcanrefresh", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "lvcanreset", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "lvcancheck", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        // camera
        { "camgetver", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "requestCamera", 4, CALLABLE_4(CProtocolTeltonika, FnRequestCamera, std::string, std::string, uint64_t, int) },
        { "getCamReqStatus", 1, CALLABLE_1(CProtocolTeltonika, FnGetCameraRequestStatus, unsigned) },
/*        {"", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0)},
        { "", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },
        { "", 0, CALLABLE_0(CProtocolTeltonika, FnCommand0) },*/
    };

    nNbFuntions = CountOf(s_Functions);
    return s_Functions;
}

CProtocolTeltonika::EHandleReturn CProtocolTeltonika::HandleWelcome(CDevice* pDevice) const
{
    auto& buffer = pDevice->GetBuffer();
    const uint16_t header = NtoH(buffer.GetValue<uint16_t>(0));
    const int TotalLength = static_cast<int>(header + sizeof(header));
    if (TotalLength > buffer.GetTotalLength())
    {
        LogErr("WioT", "[%s] Internal error : message total length (%d) is bigger than buffer size (%d)",
            Name(), TotalLength, buffer.GetTotalLength());
        return EHandleReturn::Error;
    }
    // it is a welcome message
    if (buffer.GetAvailableCount() < TotalLength)
    {
        // not enough data
        return EHandleReturn::NotEnoughData;
    }
    std::string IMEI(reinterpret_cast<const char*>(buffer.Buffer() + sizeof(header)), header);
    if (!pDevice->SetDeviceId(IMEI))
    {
        LogInf("WioT", "Could not set device IMEI %s", IMEI.c_str());
        return EHandleReturn::Error;
    }

    LogInf("WioT", "Connected to device %s", IMEI.c_str());
    uint8_t response = 1;
    pDevice->Send(&response, sizeof(response));
    buffer.Consume(TotalLength);

/*
    // now, we send a getver command to know what sort of device we are in front of
    CAutoPtr<CDeviceRequest> pRequest(new CTeltonikaRequest(pDevice, "getver"), false);
    pDevice->PushRequest(pRequest);
 */
    return EHandleReturn::Success;
}

CProtocolTeltonika::EHandleReturn CProtocolTeltonika::HandleMessage(CDevice* pDevice) const
{
    auto& buffer = pDevice->GetBuffer();

    if (buffer.GetAvailableCount() == 1 &&
        buffer.GetValue<uint8_t>(0)==0xff)
    {
        buffer.Consume(1);
        // it is a ping frame
        return EHandleReturn::NotEnoughData;
    }

    if (buffer.GetAvailableCount() < 8)
    {
        // not enough data
        return EHandleReturn::NotEnoughData;
    }

    const int DataFieldLength = NtoH(buffer.GetValue<uint32_t>(4));
    const int TotalLength = 8 + DataFieldLength + 4;
    if (TotalLength > buffer.GetTotalLength())
    {
        LogErr("WioT", "[%s] Internal error : message total length (%d) is bigger than buffer size (%d)",
            Name(), TotalLength, buffer.GetTotalLength());
        return EHandleReturn::Error;
    }

    if (buffer.GetAvailableCount() < TotalLength)
    {
        // not enough data
        return EHandleReturn::NotEnoughData;
    }

    const uint8_t CodecID = buffer.GetValue<uint8_t>(8);
    const uint8_t DataCount1 = buffer.GetValue<uint8_t>(9);
    const uint8_t DataCount2 = buffer.GetValue<uint8_t>(8 + DataFieldLength - 1);

    if (DataCount1 != DataCount2)
    {
        LogErr("WioT", "[%s] Protocol error : DataCount1 (%02x) <> DataCount2 (%02x)", Name(), DataCount1, DataCount2);
        return EHandleReturn::Error;
    }

    const uint32_t CRC = NtoH(buffer.GetValue<uint32_t>(8 + DataFieldLength));
    const uint16_t CalcCRC = ComputeCRC(buffer.Buffer() + 8, DataFieldLength);
    if (CRC != static_cast<uint32_t>(CalcCRC))
    {
        LogErr("WioT", "[%s] Protocol error : calculated CRC (%04x) <> expected CRC (%04x)", Name(), CalcCRC, CRC);
        return EHandleReturn::Error;
    }
    switch (CodecID)
    {
    case 0x08:
        // minus 3 cause CodecID, DataCount1, DataCount2 are included
        Decode8(pDevice, DataCount1, DataFieldLength - 3);
        break;
    case 0x8E:
        // minus 3 cause CodecID, DataCount1, DataCount2 are included
        Decode8Extended(pDevice, DataCount1, DataFieldLength - 3);
        break;
    case 0x0C:
        // minus 3 cause CodecID, DataCount1, DataCount2 are included
        Decode12(pDevice, DataCount1, DataFieldLength - 3);
        break;
    default:
        LogErr("WioT", "[%s] Unrecognized Codec Id %02x", Name(), CodecID);
        break;
    }
	buffer.Consume(TotalLength);

	return EHandleReturn::Success;
}

template<typename IOcount_t, typename IO_t>
static void ReadIO(CArrayData& ExtraData, int& n, int& nOffset, const CDeviceBuffer& buffer)
{
    const int NbIO = NtoH(buffer.ReadValue<IOcount_t>(nOffset));
#if 0
#define SAVE_AVL
#endif

#ifdef SAVE_AVL
    FILE* file = nullptr;
    file = fopen("avl.txt", "at");
#endif
    for (int i = 0; i < NbIO; ++i)
    {
        const IOcount_t IOidx = NtoH(buffer.ReadValue<IOcount_t>(nOffset));
        const IO_t Value = NtoH(buffer.ReadValue<IO_t>(nOffset));
        char Name[64];
        sprintf(Name, "%u", IOidx);
        ExtraData[n].Name = Name;
        ExtraData[n].Value = CValue(Value);
#ifdef SAVE_AVL
        if (file)
        {
            fprintf(file, "%s=%s\n", Name, ExtraData[n].Value.ToString().c_str());
        }
#endif
        n++;
    }
#ifdef SAVE_AVL
    if (file)
    {
        fclose(file);
    }
#endif
}

template<typename IOcount_t, bool IncludeVariableLengthIO>
int CProtocolTeltonika::Decode8_(CDevice* pDevice, const int nDataCount, const int nDataLength) const
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
                    Value=std::string(reinterpret_cast<const char*>(buffer.Buffer() + nOffset), NXthLength);
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

int CProtocolTeltonika::Decode8(CDevice* pDevice, const int nDataCount, const int nDataLength) const
{
    return Decode8_<uint8_t, false>(pDevice, nDataCount, nDataLength);
}

int CProtocolTeltonika::Decode8Extended(CDevice* pDevice, const int nDataCount, const int nDataLength) const
{
    return Decode8_<uint16_t, true>(pDevice, nDataCount, nDataLength);
}

int CProtocolTeltonika::Decode12(CDevice* pDevice, const int nDataCount, const int nDataLength) const
{
    const auto& buffer = pDevice->GetBuffer();
    constexpr int StartOffset = 10;
    int nOffset = StartOffset;

    // command or response?
    const uint8_t Type = NtoH(buffer.ReadValue<uint8_t>(nOffset));
    const uint32_t Length = NtoH(buffer.ReadValue<uint32_t>(nOffset));
    const std::string Data(reinterpret_cast<const char*>(buffer.Buffer() + nOffset), Length);

    switch (Type)
    {
    case 0x05:
        OnCommand(pDevice, Data);
        break;
    case 0x06:
        OnResponse(pDevice, Data);
        break;
    }
    return 1;
}

bool CProtocolTeltonika::SendCommand(CDevice* pDevice, const std::string& zCommand) const
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

    LogDebug("WioT", "Device %s : sending command \"%s\"", pDevice->GetDeviceId().c_str(), zCommand.c_str());

    return pDevice->Send(buffer.Buffer(), buffer.GetAvailableCount());
}

bool CProtocolTeltonika::OnCommand(CDevice* pDevice, const std::string& zCommand) const
{
    LogWarn("WioT", "Device %s : Getting command \"%s\" from device but command are not supported.", pDevice->GetDeviceId().c_str(), zCommand.c_str());
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


bool CProtocolTeltonika::OnResponse(CDevice* pDevice, const std::string& zResponse) const
{
    CAutoPtr<CDeviceRequest> pRequest = pDevice->GetCurrentRequest();

    if (pRequest.Empty())
    { 
        return true;
    }
    CTeltonikaRequest* pTeltReq = static_cast<CTeltonikaRequest*>(static_cast<CDeviceRequest*>(pRequest));

    if (pTeltReq->GetCommand() == "getver")
    {
        CRegexpA regex("Hw:([^ ]*)");

        MatchResult result;
        result = regex.Match(zResponse.c_str());
        if (result.IsMatched())
        {
            std::string zType = "Teltonika " + zResponse.substr(result.GetGroupStart(1), result.GetGroupEnd(1) - result.GetGroupStart(1));
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
    else if (zResponse.compare(0, 6, "Error:") == 0 ||
             zResponse.compare(0, 13, "Video request") == 0 ||
             zResponse.compare(0, 13, "Photo request") == 0)
    {
        bRet = false;
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

uint16_t CProtocolTeltonika::ComputeCRC(const uint8_t* pData, const int nLength)
{
    unsigned CRC = 0;
    
    for (int i = 0; i < nLength; ++i)
    {
        CRC ^= pData[i];

        for (int j = 0; j < 8; ++j)
        {
            bool Carry = (CRC & 1) != 0;
            CRC >>= 1;
            if (Carry)
            {
                CRC ^= 0xa001;
            }
        }
    }

    return static_cast<uint16_t>(CRC);
}

int CProtocolTeltonika::FnCommand0(const char* zFuncName, CDevice* pDevice, std::string &ret)
{
    if (!pDevice)
    {
        ret = "Device is null";
        return -1;
    }
    CAutoPtr<CTeltonikaRequest> pRequest(new CTeltonikaRequest(pDevice, zFuncName), false);
    if (!pDevice->PushRequest(pRequest))
    {
        ret = "Cannot create request";
        return -1;
    }

    if (!pRequest->WaitForResponse(pDevice->GetRequestTimeout()))
    {
        pDevice->CancelRequest(pRequest);
        ret = "Timeout";
        return -1;
    }
    ret = pRequest->GetResponse();
    return 0;
}

int CProtocolTeltonika::FnSendCommand(const char* zFuncName, CDevice* pDevice, std::string& ret, const std::string &zCommand)
{
    if (!pDevice)
    {
        ret = "Device is null";
        return -1;
    }

    CAutoPtr<CTeltonikaRequest> pRequest(new CTeltonikaRequest(pDevice, zCommand), false);
    if (!pDevice->PushRequest(pRequest))
    {
        ret = "Cannot create request";
        return -1;
    }

    if (!pRequest->WaitForResponse(pDevice->GetRequestTimeout()))
    {
        pDevice->CancelRequest(pRequest);
        ret = "Timeout";
        return -1;
    }
    ret = pRequest->GetResponse();
    return 0;
}

template<class T>
int CProtocolTeltonika::FnCommand1(const char* zFuncName, CDevice* pDevice, std::string& ret, T Value1)
{
    if (!pDevice)
    {
        ret = "Device is null";
        return -1;
    }
    std::ostringstream oss;

    oss << zFuncName << " " << Value1;
    CAutoPtr<CTeltonikaRequest> pRequest(new CTeltonikaRequest(pDevice, oss.str()), false);
    if (!pDevice->PushRequest(pRequest))
    {
        ret = "Cannot create request";
        return -1;
    }

    if (!pRequest->WaitForResponse(pDevice->GetRequestTimeout()))
    {
        pDevice->CancelRequest(pRequest);
        ret = "Timeout";
        return -1;
    }
    ret = pRequest->GetResponse();
    return 0;
}

int CProtocolTeltonika::FnRequestCamera(const char* zFuncName, CDevice* pDevice, std::string& ret, const std::string& zType, const std::string& zSource, uint64_t uTimestamp, int nDuration)
{
    if (!pDevice)
    {
        ret = "Device is null";
        return -1;
    }

    int nSource;
    if (zSource == "front")
    {
        nSource = Source_Front;
    }
    else if (zSource == "rear")
    {
        nSource = Source_Rear;
    }
    else
    {
        ret = "Bad parameter for Source. Allowed values are \"front\" or \"rear\"";
        return -1;
    }

    int nType;
    if (zType == "video")
    {
        nType = Type_Video;
    }
    else if (zType == "photo")
    {
        nType = Type_Photo;
    }
    else
    {
        ret = "Bad parameter for Type. Allowed values are \"photo\" or \"video\"";
        return -1;
    }

    if (nDuration < 0 || nDuration>30)
    {
        ret = "Bad parameter for Duration. Value must range from 0 to 30s";
        return -1;

    }

    std::ostringstream oss;

    oss << "camreq:" << nType << "," << nSource << "," << uTimestamp << "," << nDuration;

    if (!ms_Instance.m_zCameraRequestServerAddressIP.empty())
    {
        oss << "," << ms_Instance.m_zCameraRequestServerAddressIP;
        if (ms_Instance.m_nCameraRequestServerPort > 0 && ms_Instance.m_nCameraRequestServerPort < 65536)
        {
            oss << "," << ms_Instance.m_nCameraRequestServerPort;
        }
    }

    CAutoPtr<CTeltonikaRequest> pRequest(new CTeltonikaRequest(pDevice, oss.str()), false);
    if (!pDevice->PushRequest(pRequest))
    {
        ret = "Cannot create request";
        return -1;
    }

    if (!pRequest->WaitForResponse(pDevice->GetRequestTimeout()))
    {
        pDevice->CancelRequest(pRequest);
        ret = "Timeout";
        return -1;
    }
    ret = pRequest->GetResponse();
    if (ret.find("Preparing to send file from timestamp") == std::string::npos)
    {
        return -1;
    }

    // clear oss to reuse it
    oss.str("");
    if (!CProtocolDualCam::Instance().GetCameraBasePath().empty())
    {
        oss << CProtocolDualCam::Instance().GetCameraBasePath() << "/";
    }
    oss << zType << "_" << zSource << "_" << uTimestamp << "_" << nDuration << "_" << pDevice->GetDeviceId().c_str() << ((nType == Type_Video) ? ".mp4" : ".jpg");

    // request is ok;
    LCamReqId lCamReqId = CProtocolDualCam::FollowCamRequest(pDevice, nType, nSource, uTimestamp, nDuration, oss.str().c_str());
    if (lCamReqId == InvalidCamReqId)
    {
        ret = "Erreur requesting camera request id";
        return -1;
    }

    oss.str("");
    oss << "{\"ReturnStatus\":\"OK\",\"CamReqId\":" << lCamReqId << ",\"DeviceResponse\":\"" << ret << "\"}";

    ret = oss.str();
    return 0;
}

int CProtocolTeltonika::FnGetCameraRequestStatus(const char* zFuncName, CDevice* pDevice, std::string& ret, unsigned lCamReqId)
{
    if (!CProtocolDualCam::GetCamRequestStatus(lCamReqId, ret))
    {
        return -1;
    }
    return 0;
}

duk_ret_t CProtocolTeltonika::FnGetParam(const char* zFuncName, CDevice* pDevice, std::string& ret, duk_context* ctx)
{
    if (!pDevice)
    {
        ret = "Device is null";
        return -1;
    }
    if (!zFuncName)
    {
        ret = "Function name is null";
        return -1;
    }
    if (!ctx)
    {
        ret = "Duktape context is null";
        return -1;
    }

    if (duk_get_top(ctx) < 1)
    {
        ret="getparam requires at least 1 arguments";
        return -1;
    }


    std::ostringstream oss;

    if (!duk_is_array(ctx, 0))
    {
        oss << zFuncName << " " << duk_get_int(ctx, 0);
    }
    else
    {
        oss << zFuncName;

        const duk_size_t nNbArguments = duk_get_length(ctx, 0);
        bool bFirst = true;
        for (duk_int_t i = 0; i < static_cast<duk_int_t>(nNbArguments); i++)
        {
            if (bFirst)
            {
                bFirst = false;
                oss << " ";
            }
            else
            {
                oss << ";";
            }

            duk_get_prop_index(ctx, 0, i);
            oss << duk_get_int(ctx, -1);
            duk_pop(ctx);
        }
    }

    CAutoPtr<CTeltonikaRequest> pRequest(new CTeltonikaRequest(pDevice, oss.str()), false);
    if (!pDevice->PushRequest(pRequest))
    {
        ret = "Cannot create request";
        return -1;
    }

    if (!pRequest->WaitForResponse(pDevice->GetRequestTimeout()))
    {
        pDevice->CancelRequest(pRequest);
        ret = "Timeout";
        return -1;
    }
    ret = pRequest->GetResponse();
    return 0;
}

duk_ret_t CProtocolTeltonika::FnSetParam(const char* zFuncName, CDevice* pDevice, std::string& ret, duk_context* ctx)
{
    if (!pDevice)
    {
        ret = "Device is null";
        return -1;
    }
    if (!zFuncName)
    {
        ret = "Function name is null";
        return -1;
    }
    if (!ctx)
    {
        ret = "Duktape context is null";
        return -1;
    }

    const duk_size_t nNbArguments = duk_get_top(ctx);
    if (nNbArguments < 1 ||
        nNbArguments > 2 ||
        (nNbArguments == 1 && !duk_is_object(ctx, 0)))
    {
        ret="setparam requires 1 array or 2 arguments";
        return -1;
    }

    std::ostringstream oss;
    oss << zFuncName;

    if (nNbArguments == 1)
    {
        bool bFirst = true;
        duk_enum(ctx, 0, DUK_ENUM_INCLUDE_SYMBOLS);  /* Pushes enumerator object. */
        while (duk_next(ctx, -1, 1 /*get_value*/))
        {
            /* -1 points to enumerator, here top of stack */
            /* Each time duk_enum() finds a new key/value pair, it
             * gets pushed to the value stack.  So here the stack
             * top is [ ... enum key value ].  Enum is at index -3,
             * key at -2, value at -1, all relative to stack top.
             */

            if (bFirst)
            {
                bFirst = false;
                oss << " ";
            }
            else
            {
                oss << ";";
            }

            oss << duk_safe_to_string(ctx, -2) << ":" << duk_safe_to_string(ctx, -1);

            /* When you're done with the key/value, pop them off. */
            duk_pop_2(ctx);
        }
        duk_pop(ctx);  /* Pop enumerator object. */
    }
    else
    {
        duk_to_int(ctx, 0);
        duk_to_string(ctx, 1);
        oss << " " << duk_get_int(ctx, 0) << ":" << duk_get_string(ctx, 1);
    }


    CAutoPtr<CTeltonikaRequest> pRequest(new CTeltonikaRequest(pDevice, oss.str()), false);
    if (!pDevice->PushRequest(pRequest))
    {
        pDevice->CancelRequest(pRequest);
        ret = "Cannot create request";
        return -1;
    }

    if (!pRequest->WaitForResponse(pDevice->GetRequestTimeout()))
    {
        pDevice->CancelRequest(pRequest);
        ret = "Timeout";
        return -1;
    }
    ret = pRequest->GetResponse();
    return 0;
}

const char* CProtocolTeltonika::Name()
{
    return "Teltonika";
}

void CProtocolTeltonika::SetCameraRequestServerAddressIP(const std::string& zAddressIP)
{
    ms_Instance.m_zCameraRequestServerAddressIP = zAddressIP;
}

const std::string& CProtocolTeltonika::GetCameraRequestServerAddressIP()
{
    return ms_Instance.m_zCameraRequestServerAddressIP;
}

void CProtocolTeltonika::SetCameraRequestServerPort(int nPort)
{
    ms_Instance.m_nCameraRequestServerPort = nPort;
}

int CProtocolTeltonika::GetCameraRequestServerPort()
{
    return ms_Instance.m_nCameraRequestServerPort;
}
