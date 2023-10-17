#include "WioTMobility.h"
#include "CommandManager.h"
#include "SecurityManager.h"
#include <memory>

#include <cstdint>
#include <iostream>
#include <vector>

constexpr int InitialQueueSize = 256;
CCommandManager::CCommandManager()
    : m_nStartedCount(0)
    , m_nListenPort(2022)
    , m_msgQueue(InitialQueueSize)
    , m_bWantStop(false)
{
}

CCommandManager::~CCommandManager()
{
    ICommandMessage* pMessage;
    while (m_msgQueue.pop(pMessage))
    {
        if (pMessage)
        {
            delete pMessage;
        }
    }
}

bool CCommandManager::Start()
{
    m_bWantStop = false;
    struct TLauncher
    {
        CCommandManager* pManager;

        TLauncher(CCommandManager* __pManager) : pManager(__pManager)
        {}

        void operator()()
        {
            pManager->Thread();
        }
    } launcher(this);

    m_Thread = boost::thread{ launcher };

    return true;
}

bool CCommandManager::Stop()
{
    LogInf("WioT", "[CMDMAN] Received signal to quit");
    m_bWantStop = true;

    for (;;)
    {
        if (m_Thread.timed_join(boost::chrono::seconds(WaitThreadTimeout)))
        {
            break;
        }
        LogWarn("WioT", "[CMDMAN] Waiting for thread to stop...");
    }
    return true;
}

CCommandManager& CCommandManager::GetInstance()
{
    static CCommandManager s_Instance;
    return s_Instance;
}

bool CCommandManager::OnCommandPromptEnded(CCommandPrompt* pPrompt)
{
    if (!pPrompt)
    {
        return false;
    }

    {
        bool bFound = false;
        boost::mutex::scoped_lock lock(m_Mutex);
        for (auto iter = m_Prompts.begin(); iter != m_Prompts.end(); ++iter)
        {
            if (*iter == pPrompt)
            {
                m_Prompts.erase(iter);
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            return false;
        }
    }

    class CMsgCmdPromptEnded : public ICommandMessage
    {
    public:
        CMsgCmdPromptEnded(CCommandPrompt* pPrompt, CCommandManager *pManager)
            : m_pPrompt(pPrompt)
            , m_pManager(pManager)
        {}
        virtual ~CMsgCmdPromptEnded()
        {}

        bool Process() override
        {
            if (m_pPrompt)
            {
                m_pPrompt->Join();
                delete m_pPrompt;

                boost::mutex::scoped_lock lock(m_pManager->m_Mutex);
                --m_pManager->m_nStartedCount;
            }
            return true;
        }
    private:
        CCommandPrompt* m_pPrompt;
        CCommandManager* m_pManager;
    };
    CMsgCmdPromptEnded* pMessage = new CMsgCmdPromptEnded(pPrompt, this);
    if (!m_msgQueue.push(pMessage))
    {
        LogInf("WioT", "[CMDMAN] Cannot queue message");
        delete pMessage;
        return false;
    }

    return true;
}

void CCommandManager::Thread()
{
    LogInf("WioT", "[CMDMAN] Entering thread");

    socket_t listenfd = CreateServerSocket();

    if (listenfd != INVALID_SOCKET)
    {
        while (!m_bWantStop)
        {
            fd_set in;

            FD_ZERO(&in);
            FD_SET(listenfd, &in);

            timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            int res = select(listenfd + 1, &in, nullptr, nullptr, &tv);
            if (res > 0)
            {
                HandleConnection(listenfd, SecurityManager.GetClientContext());
            }

            ConsumeMessage();
        }
        close(listenfd);
    }
    {
        boost::mutex::scoped_lock lock(m_Mutex);

        for (CCommandPrompt *pPrompt : m_Prompts)
        {
            pPrompt->Stop();
        }
    }

    WaitForAllPromptStopped();

    LogInf("WioT", "[CMDMAN] Leaving thread");
}

void CCommandManager::ConsumeMessage()
{
    ICommandMessage* pMessage;
    while (m_msgQueue.pop(pMessage))
    {
        if (pMessage)
        {
            std::unique_ptr<ICommandMessage> __pMessage(pMessage);
            pMessage->Process();
        }
    }
}

bool CCommandManager::HandleConnection(socket_t listenfd, SSL_CTX* pCTX)
{
    struct sockaddr_in peer_addr;
    socklen_t sockaddr_len = sizeof(peer_addr);
    int connfd = accept(listenfd, reinterpret_cast<sockaddr*>(&peer_addr), &sockaddr_len);
    if (connfd < 0)
    {
        const int err = errno;
        LogErr("WioT", "[CMDMAN] Failed to accept connection.\n"
            "Error %d : %s",
            err, strerror(err));
        return false;
    }

    if (SecurityManager.IsBlacklisted(peer_addr))
    {
        close(connfd);
        LogErr("WioT", "[CMDMAN] Rejected connection from blacklisted address %d.%d.%d.%d",
            (peer_addr.sin_addr.s_addr) & 0xff,
            (peer_addr.sin_addr.s_addr >> 8) & 0xff,
            (peer_addr.sin_addr.s_addr >> 16) & 0xff,
            (peer_addr.sin_addr.s_addr >> 24) & 0xff);
        return false;
    }

    CSocket* pSocket;
    if (pCTX)
    {
        pSocket = new CSocketSSL(pCTX);
    }
    else
    {
        LogWarn("WioT", "[CMDMAN] Command communication are not securized");
        pSocket = new CSocket;
    }
    if (!AddCommandPrompt(pSocket, connfd))
    {
        delete pSocket;
        LogErr("WioT", "[CMDMAN] Failed to append a new command prompt");
        return false;
    }
    LogInf("WioT", "[CMDMAN] New connection from %d.%d.%d.%d:%d",
        (peer_addr.sin_addr.s_addr) & 0xff,
        (peer_addr.sin_addr.s_addr >> 8) & 0xff,
        (peer_addr.sin_addr.s_addr >> 16) & 0xff,
        (peer_addr.sin_addr.s_addr >> 24) & 0xff,
        ntohs(peer_addr.sin_port));

    return true;
}

bool CCommandManager::AddCommandPrompt(CSocket* pSocket, socket_t connfd)
{
    CCommandPrompt* pPrompt = new CCommandPrompt;

    if (!pPrompt->Start(pSocket, connfd))
    {
        delete pPrompt;
        return false;
    }

    boost::mutex::scoped_lock lock(m_Mutex);
    m_Prompts.push_back(pPrompt);
    m_nStartedCount++;

    return true;
}

bool CCommandManager::WaitForAllPromptStopped()
{
    m_Mutex.lock();
    if (m_nStartedCount > 0)
    {
        LogDebug("WioT", "Left %d command prompt thread running", m_nStartedCount);

        while (m_nStartedCount)
        {
            m_Mutex.unlock();

            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            ConsumeMessage();

            m_Mutex.lock();
        }
    }
    else
    {
        LogDebug("WioT", "No command prompt thread running");
    }
    m_Mutex.unlock();

    LogInf("WioT", "All command prompt are stopped");

    return true;
}

socket_t CCommandManager::CreateServerSocket()
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    int reuse = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
    {
        const int err = errno;
        LogErr("WioT", "[CMDMAN] Cannot set SO_REUSEADDR on port %d.\n"
            "Error %d : %s",
            m_nListenPort,
            err, strerror(err));

        close(listenfd);
        return INVALID_SOCKET;
    }

#ifdef SO_REUSEPORT
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)
    {
        const int err = errno;
        LogErr("WioT", "[CMDMAN] Cannot set SO_REUSEPORT on port %d.\n"
            "Error %d : %s",
            m_nListenPort,
            err, strerror(err));

        close(listenfd);
        return INVALID_SOCKET;
    }
#endif
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(static_cast<uint16_t>(m_nListenPort));
    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
    {
        const int err = errno;
        LogErr("WioT", "[CMDMAN] Cannot bind on port %d.\n"
            "Error %d : %s",
            m_nListenPort,
            err, strerror(err));

        close(listenfd);
        return INVALID_SOCKET;
    }

    // let's listen
    if (listen(listenfd, -1) != 0)
    {
        const int err = errno;
        LogErr("WioT", "[CMDMAN] Error while listening port %d.\n"
            "Error %d : %s",
            m_nListenPort,
            err, strerror(err));

        close(listenfd);
        return INVALID_SOCKET;
    }

    return listenfd;
}
