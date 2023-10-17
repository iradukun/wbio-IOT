#include "WioTMobility.h"
#include "CommandPrompt.h"
#include "CommandManager.h"
#include "Socket.h"
#include "stdexcept"

#include <poll.h>
#include <cerrno>
#include <cstring>
#include <string>

#include "IScriptModule.h"
#include "ScriptModuleServer.h"
#include "ScriptModuleDevice.h"
#include "ScriptModuleCore.h"

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

constexpr const char * c_WioTThis = DUK_HIDDEN_SYMBOL("WioTThis");
constexpr char EndOfFile = 0x1a;

CCommandPrompt* IScriptModule::GetPrompt(duk_context* ctx)
{
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, c_WioTThis);
    CCommandPrompt* pJavascript = reinterpret_cast<CCommandPrompt*>(duk_get_pointer(ctx, -1));
    duk_pop_2(ctx);

    return pJavascript;
}

CCommandPrompt::CCommandPrompt()
    : m_pSocket(nullptr)
    , m_bWantStop(false)
    , m_nCurrentOffset(0)
{
}

CCommandPrompt::~CCommandPrompt()
{
    if (m_pSocket)
    {
        delete m_pSocket;
    }
}

bool CCommandPrompt::Start(CSocket* pSocket, socket_t connfd)
{
    if (!pSocket)
    {
        return false;
    }
    m_pSocket = pSocket;
    struct TLauncher
    {
        CCommandPrompt* pPool;
        socket_t connfd;

        TLauncher(CCommandPrompt* __pPool, socket_t __connfd)
            : pPool(__pPool)
            , connfd(__connfd)
        {}

        void operator()()
        {
            pPool->Thread(connfd);
        }
    } launcher(this, connfd);

    m_Thread = boost::thread{ launcher };

    return true;
}

bool CCommandPrompt::Stop()
{
    m_bWantStop = true;
    return false;
}

bool CCommandPrompt::Join()
{
    for (;;)
    {
        if (m_Thread.timed_join(boost::chrono::seconds(WaitThreadTimeout)))
        {
            break;
        }
        LogWarn("WioT", "[CMDPROMPT] Waiting for thread to stop...");
    }
    return false;
}

void CCommandPrompt::Print(const std::string& msg)
{
    if (m_pSocket)
    {
        size_t offset = 0;
        auto length = msg.length();
        while (offset < length)
        {
            auto ret = m_pSocket->Send(msg.c_str() + offset, length - offset);
            if (ret <= 0)
            {
                LogErr("WioT", "[CMDPROMPT] Error sending response (%d bytes length)...", length - offset);
                break;
            }
            offset += ret;
        }
        const char cr = '\n';
        m_pSocket->Send(&cr, 1);
    }
}

void CCommandPrompt::PrintError(const char *zReturnStatus, const char *zErrorMessage)
{
    auto builder = bsoncxx::builder::stream::document{};
    auto context = builder << "ReturnStatus" << zReturnStatus;
    if (zErrorMessage)
    {
        context << "ErrorMessage" << zErrorMessage;
    }
    bsoncxx::document::value infos=context << finalize;
    Print(bsoncxx::to_json(infos));
}

void CCommandPrompt::Thread(socket_t connfd)
{
    if (m_pSocket &&
        !m_pSocket->OnAccept(connfd))
    {
        struct sockaddr_in peer_addr;
        socklen_t sockaddr_len = sizeof(peer_addr);
        getpeername(connfd, reinterpret_cast<sockaddr*>(&peer_addr), &sockaddr_len);
        close(connfd);
        LogErr("WioT", "[CMDMAN] Connection refused from %d.%d.%d.%d:%d",
            (peer_addr.sin_addr.s_addr) & 0xff,
            (peer_addr.sin_addr.s_addr >> 8) & 0xff,
            (peer_addr.sin_addr.s_addr >> 16) & 0xff,
            (peer_addr.sin_addr.s_addr >> 24) & 0xff,
            ntohs(peer_addr.sin_port));
    }
    else
    {
        DoWork();
    }
    CommandManager.OnCommandPromptEnded(this);
}

void CCommandPrompt::DoWork()
{
    LogInf("WioT", "[CMDPROMPT] Entering thread");

    duk_context* ctx = duk_create_heap(nullptr, nullptr, nullptr, this, FatalHandler);
    InitializeJavascriptContext(ctx);

    while (!m_bWantStop)
    {
        pollfd fds;

        fds.fd = m_pSocket->GetSocket();
        fds.events = POLLIN;
        fds.revents = 0;

        int res = poll(&fds, 1, 1000);
        if (res > 0)
        {
            // there is incomming data...
            if (fds.revents & POLLIN &&
                !OnIncomingData(ctx))
            {
                break;
            }
        }
    }

    FinalizeJavascriptContext(ctx);
    duk_destroy_heap(ctx);

    LogInf("WioT", "[CMDPROMPT] Leaving thread");
}

bool CCommandPrompt::OnIncomingData(duk_context* ctx)
{
    if (m_nCurrentOffset >= static_cast<int>(sizeof(m_Buffer)))
    {
        LogInf("WioT", "[CMDPROMPT] Line too long. Reset connection.");
        return false;
    }

    int ret = static_cast<int>(m_pSocket->Receive(m_Buffer + m_nCurrentOffset, sizeof(m_Buffer) - m_nCurrentOffset - 1));
    if (ret > 0)
    {
        m_nCurrentOffset += ret;
        for (int i = 0; i < m_nCurrentOffset; ++i)
        {
            if (m_Buffer[i] == EndOfFile)
            {
                m_Buffer[i] = '\0';

                if (i > 0)
                {
                    LogDebug("WioT", "[CMDPROMPT] Evaluating :\n%s", m_Buffer);
                    EvalString(ctx, m_Buffer);
                }
                if (i < m_nCurrentOffset - 1)
                {
                    memmove(m_Buffer, m_Buffer + i + 1, m_nCurrentOffset - i - 1);
                    m_nCurrentOffset -= i + 1;
                }
                else
                {
                    m_nCurrentOffset = 0;
                }
                // -1 because of the ++i
                i = -1;
            }
        }

        // for the moment, do echo
//        m_pSocket->Send(line, ret);
        return true;
    }

    if (ret == 0)
    {
        LogInf("WioT", "Command prompt client closed connection");
    }
    else
    {
        const int err = m_pSocket->GetErrorCode(ret);
        const std::string zErr = m_pSocket->GetErrorString(err);
        LogErr("WioT", "Error while receiving data from command prompt client.\nError %d : %s", err, zErr.c_str());
    }

    return false;
}

static IScriptModule* const gs_Scripts[]
{
    & CScriptModuleCore::GetInstance(),
    & CScriptModuleServer::GetInstance(),
    & CScriptModuleDevice::GetInstance(),
};
constexpr int ScriptCount = CountOf(gs_Scripts);

void CCommandPrompt::InitializeJavascriptContext(duk_context* ctx)
{
    duk_push_global_object(ctx);

    duk_push_pointer(ctx, this);
    duk_put_prop_string(ctx, -2, c_WioTThis);

    duk_push_object(ctx);

    for (int i = 0; i < ScriptCount; ++i)
    {
        IScriptModule* pModule = gs_Scripts[i];
        if (!pModule)
        {
            continue;
        }
        const char * zModuleName = pModule->GetName();
        if (zModuleName)
        {
            duk_push_object(ctx);
        }
        int nFuncCount;
        const TFunction* pFunctions = pModule->GetFunctions(nFuncCount);

        for (int i = 0; i < nFuncCount; ++i)
        {
            duk_push_c_function(ctx, pFunctions[i].Proc, pFunctions[i].nArgCount);
            duk_put_prop_string(ctx, -2, pFunctions[i].zFunctionName);
        }
        if (zModuleName)
        {
            duk_put_prop_string(ctx, -2, zModuleName);
        }
        pModule->PrivateInitialization(ctx);
    }

    duk_put_prop_string(ctx, -2, "WioT");
    duk_pop(ctx);
}

void CCommandPrompt::FinalizeJavascriptContext(duk_context* ctx)
{
    for (int i = 0; i < ScriptCount; ++i)
    {
        IScriptModule* pModule = gs_Scripts[i];
        if (!pModule)
        {
            continue;
        }
        pModule->PrivateFinalization(ctx);
    }
}

void CCommandPrompt::EvalString(duk_context* ctx, const char *zScript)
{
    try
    {
        duk_eval_string(ctx, zScript);
        duk_eval_string(ctx, "\n");
    }
    catch (std::runtime_error& e)
    {
        LogDebug("WioT", "[CMDPROMPT] %s", e.what());
        Print(e.what());
    }
}

void CCommandPrompt::FatalHandler(void* udata, const char* msg)
{
    DUK_UNREF(udata);
    DUK_UNREF(msg);

    msg = msg ? msg : "~~No information~~";
    throw std::runtime_error(msg);
}
