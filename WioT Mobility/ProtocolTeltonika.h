#pragma once

#include "ProtocolBase.h"
#include "duktape.h"

class CProtocolTeltonika
    : public CProtocolBase
{
private:
    CProtocolTeltonika();
    ~CProtocolTeltonika();

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
    EHandleReturn HandleWelcome(CDevice* pDevice) const;
    EHandleReturn HandleMessage(CDevice* pDevice) const;

public:
    static const char* Name();

    static void SetCameraRequestServerAddressIP(const std::string& zAddressIP);
    static const std::string &GetCameraRequestServerAddressIP();
    static void SetCameraRequestServerPort(int nPort);
    static int GetCameraRequestServerPort();
private:
    int Decode8(CDevice* pDevice, const int nDataCount, const int nDataLength) const;
    int Decode8Extended(CDevice* pDevice, const int nDataCount, const int nDataLength) const;
    int Decode12(CDevice* pDevice, const int nDataCount, const int nDataLength) const;

    template<typename IOcount_t, bool IncludeVariableLengthIO>
    int Decode8_(CDevice* pDevice, const int nDataCount, const int nDataLength) const;

    bool SendCommand(CDevice* pDevice, const std::string& zCommand) const;

    bool OnCommand(CDevice* pDevice, const std::string& zCommand) const;
    bool OnResponse(CDevice* pDevice, const std::string& zResponse) const;

    static uint16_t ComputeCRC(const uint8_t* pData, const int nLength);

    static CProtocolTeltonika ms_Instance;

    static int FnSendCommand(const char* zFuncName, CDevice* pDevice, std::string& ret, const std::string& zCommand);
    static int FnCommand0(const char *zFuncName, CDevice *pDevice, std::string &ret);
    template<class T>
    static int FnCommand1(const char *zFuncName, CDevice *pDevice, std::string &ret, T Value1);
    static duk_ret_t FnGetParam(const char *zFuncName, CDevice *pDevice, std::string &ret, duk_context *ctx);
    static duk_ret_t FnSetParam(const char *zFuncName, CDevice *pDevice, std::string &ret, duk_context *ctx);
    static int FnRequestCamera(const char *zFuncName, CDevice *pDevice, std::string &ret, const std::string &zType, const std::string &zSource, uint64_t uTimestamp, int nDuration);
    static int FnGetCameraRequestStatus(const char *zFuncName, CDevice *pDevice, std::string &ret, unsigned lCamReqId);

    friend class CTeltonikaRequest;
    friend class CProtocolDualCam;

    std::string m_zCameraRequestServerAddressIP;
    int m_nCameraRequestServerPort;
};

