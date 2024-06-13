#ifndef SERVERASIO_H
#define SERVERASIO_H

#include "asio.hpp"

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <functional>
#include <optional>
#include <thread>

class SessionASIO;

struct MessageASIO
{
    std::weak_ptr<SessionASIO> session;
    std::string content;
};

class SessionASIO : public std::enable_shared_from_this<SessionASIO>
{
public:
    SessionASIO(asio::ip::tcp::socket &&socket, std::map<std::string, std::function<void (MessageASIO)>> *dispatcher, uint64_t sessionID);

    void start();
    void write(const char *data, size_t size);

private:
    void read();
    void on_read(asio::error_code error, std::size_t bytesTransferred);
    void on_write(asio::error_code error, std::size_t bytesTransferred);
    void dispatch(const std::string &line);

    static std::string encode(const char *input, size_t size);
    static std::string decode(const char *input, size_t size);

    asio::ip::tcp::socket m_socket;
    std::map<std::string, std::function<void (MessageASIO)> > *m_dispatcher;
    asio::streambuf m_incoming;
    asio::streambuf m_outgoing;

    uint64_t m_sessionID = 0;
    size_t m_totalBytesSent = 0;
    size_t m_totalBytesReceived = 0;
};

class ServerASIO
{
public:

    ServerASIO();

    int setPort(std::uint16_t port);
    void start();
    void stop();
    void attach(const std::string &command, std::function<void (MessageASIO)> &&function);

    void getLocalAddress(std::array<uint8_t, 4> *ipAddress, uint16_t *port);

private:
    void accept();
    void acceptHandler(const asio::error_code &errorCode);


    asio::io_context m_ioContext;
    std::optional<asio::ip::tcp::tcp::acceptor> m_acceptor;
    std::optional<asio::ip::tcp::tcp::socket> m_socket;
    std::map<std::string, std::function<void (MessageASIO)> > m_dispatcher;

    static uint64_t m_sessionID;
};

class StopServerASIOGuard
{
public:
    StopServerASIOGuard(ServerASIO *server, std::thread *serverThread)
    {
        m_server = server;
        m_serverThread = serverThread;
    }
    ~StopServerASIOGuard()
    {
        if (m_server) m_server->stop();
        if (m_serverThread) m_serverThread->join();
        delete m_server;
        delete m_serverThread;
    }
private:
    ServerASIO *m_server = nullptr;
    std::thread *m_serverThread = nullptr;
};

#endif // SERVERASIO_H
