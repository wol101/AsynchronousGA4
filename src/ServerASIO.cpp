#include "ServerASIO.h"

#include <iostream>

using namespace std::string_literals;

uint64_t ServerASIO::m_sessionID = 0;

SessionASIO::SessionASIO(asio::ip::tcp::socket &&socket, std::map<std::string, std::function<void (MessageASIO)> > *dispatcher, uint64_t sessionID) :
    m_socket(std::move(socket))
{
    m_dispatcher = dispatcher;
    m_sessionID = sessionID;
    m_socket.set_option(asio::ip::tcp::tcp::no_delay(true));
    m_socket.set_option(asio::socket_base::linger(false, 0));
}

void SessionASIO::start()
{
    read();
}

void SessionASIO::write(const char *data, size_t size)
{
    // need to do prepare and commit for streambuf, the asio::async_write does the consume
    // the input data may need to be mutex locked for this to work
    if (!data || !size) return;
    std::string encoded = encode(data, size);
    auto view = m_outgoing.prepare(encoded.size());
    std::memcpy(view.data(), encoded.data(), encoded.size());
    m_outgoing.commit(encoded.size());
    try
    {
        // note: shared_from_this() is required here to guarantee that the SessionASIO does not vanish before the handler is used (using this on its own causes a crash)
        asio::async_write(m_socket, m_outgoing, std::bind(&SessionASIO::on_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    catch (std::exception& e)
    {
        std::cerr << __LINE__ << " asio::async_write() " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "SessionASIO::write() exception caught on line " << __LINE__ << "\n";
    }
}

void SessionASIO::read()
{
    try
    {
        // note: shared_from_this() is required here to guarantee that the SessionASIO does not vanish before the handler is used (using this on its own causes a crash)
        asio::async_read_until(m_socket, m_incoming, '\0', std::bind(&SessionASIO::on_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    catch (std::exception& e)
    {
        std::cerr << __LINE__ << " asio::async_read_until()" << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "SessionASIO::read() exception caught on line " << __LINE__ << "\n";
    }
}

void SessionASIO::on_read(asio::error_code error, std::size_t bytesTransferred)
{
    if (!error)
    {
        m_totalBytesReceived += bytesTransferred;
        asio::streambuf::const_buffers_type bufs = m_incoming.data();
        std::string str(asio::buffers_begin(bufs), asio::buffers_begin(bufs) + ptrdiff_t(bytesTransferred));
        m_incoming.consume(bytesTransferred);
        dispatch(str);
        read();
    }
}

void SessionASIO::on_write(asio::error_code error, std::size_t bytesTransferred)
{
    if (!error)
    {
        m_totalBytesSent += bytesTransferred;
    }
}

void SessionASIO::dispatch(std::string const& line)
{
    std::string decodedLine = SessionASIO::decode(line.data(), line.size());
    std::string command = decodedLine.substr(0, 8);

    if (auto it = m_dispatcher->find(command); it != m_dispatcher->cend())
    {
        auto const& entry = it->second;
        MessageASIO message;
        message.session = shared_from_this();
        message.content = decodedLine;
        entry(message);
    }
}

std::string SessionASIO::encode(const char *input, size_t size)
{
    std::string output;
    output.reserve(size * 2 + 1);
    for (size_t i = 0; i < size; i++)
    {
        if (input[i] == '\0')
        {
            output.push_back('\xff');
            output.push_back('\x1');
            continue;
        }
        if (input[i] == '\xff')
        {
            output.push_back('\xff');
            output.push_back('\x2');
            continue;
        }
        output.push_back(input[i]);
    }
    output.push_back('\0');
    return output;
}

std::string SessionASIO::decode(const char *input, size_t size)
{
    std::string output;
    output.reserve(size);
    const char *ptr = input;
    while (*ptr)
    {
        if (*ptr != '\xff')
        {
            output.push_back(*ptr);
            ptr++;
            continue;
        }
        ptr++;
        if (*ptr)
        {
            if (*ptr == '\x1')
            {
                output.push_back('\0');
                ptr++;
                continue;
            }
            if (*ptr == '\x2')
            {
                output.push_back('\xff');
                ptr++;
                continue;
            }
        }
    }
    return output;
}


ServerASIO::ServerASIO()
{
    // this is for testing. Access using netcat host port
    // echo -en "version\0" | netcat 127.0.0.1 8090
    attach("version"s, [] (MessageASIO message)
    {
        const static std::string version("ServerASIO compiled "s + __DATE__ + " "s + __TIME__ + "\r\n"s);
        if (auto sharedPtr = message.session.lock())
            sharedPtr->write(version.data(), version.size());
    });
}

int ServerASIO::setPort(std::uint16_t port)
{
    try
    {
        asio::ip::tcp::tcp::endpoint endpoint(asio::ip::tcp::tcp::v4(), port);
        m_acceptor.emplace(asio::ip::tcp::tcp::acceptor(m_ioContext, endpoint));
    }
    catch (std::exception& e)
    {
        std::cerr << __LINE__ << " " << e.what() << std::endl;
        return __LINE__;
    }
    catch (...)
    {
        std::cerr << "ServerASIO::setPort() exception caught on line " << __LINE__ << "\n";
        return __LINE__;
    }
    return 0;
}

void ServerASIO::start()
{
    accept();
    try
    {
        m_ioContext.run();
    }
    catch (std::exception& e)
    {
        std::cerr << __LINE__ << " m_ioContext.run() " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "ServerASIO::start() exception caught on line " << __LINE__ << "\n";
    }
}

void ServerASIO::stop()
{
    try
    {
        m_ioContext.stop();
    }
    catch (std::exception& e)
    {
        std::cerr << __LINE__ << " m_ioContext.stop() " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "ServerASIO::stop() exception caught on line " << __LINE__ << "\n";
    }
}

void ServerASIO::attach(const std::string &command, std::function<void (MessageASIO)> &&function)
{
    m_dispatcher.emplace(command, std::move(function));
}

void ServerASIO::accept()
{
    if (!m_acceptor.has_value())
    {
        std::cerr << "ServerASIO::accept() acceptor not initialised on line " << __LINE__ << "\n";
        return;
    }
    try
    {
        m_socket.emplace(m_ioContext);
        m_acceptor->async_accept(*m_socket, std::bind(&ServerASIO::acceptHandler, this, std::placeholders::_1));
    }
    catch (std::exception& e)
    {
        std::cerr << __LINE__ << " " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "ServerASIO::accept() exception caught on line " << __LINE__ << "\n";
    }
}

void ServerASIO::acceptHandler(const asio::error_code &errorCode)
{
    if (!errorCode)
    {
        m_sessionID++;
        auto session = std::make_shared<SessionASIO>(std::move(*m_socket), &m_dispatcher, m_sessionID);
        session->start();
        accept();
    }
    else
    {
        accept();
    }
}

void ServerASIO::getLocalAddress(std::array<uint8_t, 4> *ipAddress, uint16_t *port)
{
    if (m_acceptor.has_value())
    {
        auto address = m_acceptor->local_endpoint().address().to_v4().to_bytes();
        *port = m_acceptor->local_endpoint().port();
        *ipAddress = {address.data()[0], address.data()[1], address.data()[2], address.data()[3]};
    }
    else
    {
        *ipAddress = {0, 0, 0, 0};
        *port = 0;
    }
}

