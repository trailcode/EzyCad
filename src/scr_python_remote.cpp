#include "scr_python_remote.h"

#include <cstdio>
#include <cstring>
#include <future>
#include <memory>
#include <sstream>

#include <nlohmann/json.hpp>

#ifdef EZYCAD_HAVE_PYTHON

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t                             = SOCKET;
static constexpr socket_t k_invalid_socket = INVALID_SOCKET;
static int                closesocket_compat(socket_t s) { return ::closesocket(s); }
static int                last_socket_error() { return WSAGetLastError(); }
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_t                             = int;
static constexpr socket_t k_invalid_socket = -1;
static int                closesocket_compat(socket_t s) { return ::close(s); }
static int                last_socket_error() { return errno; }
#endif

namespace
{

bool send_all(socket_t s, const char* data, size_t len)
{
  size_t sent = 0;
  while (sent < len)
  {
#ifdef _WIN32
    const int n = ::send(s, data + sent, static_cast<int>(len - sent), 0);
#else
    const ssize_t n = ::send(s, data + sent, len - sent, 0);
#endif
    if (n <= 0)
      return false;
    sent += static_cast<size_t>(n);
  }
  return true;
}

bool recv_all(socket_t s, char* data, size_t len)
{
  size_t got = 0;
  while (got < len)
  {
#ifdef _WIN32
    const int n = ::recv(s, data + got, static_cast<int>(len - got), 0);
#else
    const ssize_t n = ::recv(s, data + got, len - got, 0);
#endif
    if (n <= 0)
      return false;
    got += static_cast<size_t>(n);
  }
  return true;
}

bool send_frame(socket_t s, const std::string& payload)
{
  if (payload.size() > 0x7fffffffu)
    return false;
  const uint32_t n    = static_cast<uint32_t>(payload.size());
  const uint32_t n_be = htonl(n);
  if (!send_all(s, reinterpret_cast<const char*>(&n_be), 4))
    return false;
  return send_all(s, payload.data(), payload.size());
}

bool recv_frame(socket_t s, std::string& payload)
{
  uint32_t n_be = 0;
  if (!recv_all(s, reinterpret_cast<char*>(&n_be), 4))
    return false;
  const uint32_t n = ntohl(n_be);
  if (n > 16u * 1024u * 1024u)
    return false;
  payload.assign(n, '\0');
  if (n == 0)
    return true;
  return recv_all(s, payload.data(), n);
}

std::string socket_error_string(const char* prefix)
{
  std::ostringstream oss;
  oss << prefix << " (err=" << last_socket_error() << ")";
  return oss.str();
}

} // namespace

bool parse_listen_arg(const std::string& arg, Python_listen_endpoint& out, std::string& error)
{
  out = Python_listen_endpoint{};
  error.clear();
  if (arg.empty())
  {
    error = "empty listen argument";
    return false;
  }

  std::string host = "127.0.0.1";
  std::string port_str;

  const size_t colon = arg.rfind(':');
  if (colon == std::string::npos)
  {
    port_str = arg;
  }
  else
  {
    host     = arg.substr(0, colon);
    port_str = arg.substr(colon + 1);
    if (host.empty())
    {
      error = "empty host in listen argument";
      return false;
    }
  }

  if (port_str.empty())
  {
    error = "empty port in listen argument";
    return false;
  }
  for (char c : port_str)
  {
    if (c < '0' || c > '9')
    {
      error = "port must be an integer";
      return false;
    }
  }

  unsigned long port_ul = 0;
  try
  {
    port_ul = std::stoul(port_str);
  }
  catch (...)
  {
    error = "invalid port";
    return false;
  }
  if (port_ul == 0 || port_ul > 65535ul)
  {
    error = "port out of range (1-65535)";
    return false;
  }

  out.host = host;
  out.port = static_cast<uint16_t>(port_ul);
  return true;
}

void Python_execution_queue::enqueue(std::string code, Completer done)
{
  Job job;
  job.code = std::move(code);
  job.done = std::move(done);
  std::lock_guard<std::mutex> lock(m_mutex);
  m_jobs.push_back(std::move(job));
}

void Python_execution_queue::process_pending(Python_console& console)
{
  std::vector<Job> jobs;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    jobs.swap(m_jobs);
  }
  for (Job& job : jobs)
  {
    Python_exec_result r = console.execute_captured(job.code);
    if (job.done)
      job.done(std::move(r));
  }
}

Python_remote_server::Python_remote_server(Python_execution_queue& queue)
    : m_queue(queue)
{
}

Python_remote_server::~Python_remote_server() { stop(); }

bool Python_remote_server::start(const Python_listen_endpoint& ep, std::string& error)
{
  error.clear();
  if (!ep.valid())
  {
    error = "invalid listen endpoint";
    return false;
  }
  if (m_running.load())
  {
    error = "already listening";
    return false;
  }

#ifdef _WIN32
  WSADATA   wsa;
  const int wsa_rc = WSAStartup(MAKEWORD(2, 2), &wsa);
  if (wsa_rc != 0)
  {
    error = "WSAStartup failed (err=" + std::to_string(wsa_rc) + ")";
    return false;
  }
  m_wsa_started = true;
#endif

  socket_t listen_sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listen_sock == k_invalid_socket)
  {
    error = socket_error_string("socket() failed");
#ifdef _WIN32
    if (m_wsa_started)
    {
      WSACleanup();
      m_wsa_started = false;
    }
#endif
    return false;
  }

  int yes = 1;
  setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(ep.port);
  if (::inet_pton(AF_INET, ep.host.c_str(), &addr.sin_addr) != 1)
  {
    error = "invalid IPv4 host: " + ep.host;
    closesocket_compat(listen_sock);
#ifdef _WIN32
    if (m_wsa_started)
    {
      WSACleanup();
      m_wsa_started = false;
    }
#endif
    return false;
  }

  if (::bind(listen_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
  {
    error = socket_error_string("bind() failed");
    closesocket_compat(listen_sock);
#ifdef _WIN32
    if (m_wsa_started)
    {
      WSACleanup();
      m_wsa_started = false;
    }
#endif
    return false;
  }

  if (::listen(listen_sock, 8) != 0)
  {
    error = socket_error_string("listen() failed");
    closesocket_compat(listen_sock);
#ifdef _WIN32
    if (m_wsa_started)
    {
      WSACleanup();
      m_wsa_started = false;
    }
#endif
    return false;
  }

  m_ep          = ep;
  m_listen_sock = static_cast<uintptr_t>(listen_sock);
  m_running.store(true);
  m_thread = std::thread([this] { accept_loop_(); });
  return true;
}

void Python_remote_server::stop()
{
  m_running.store(false);
  if (m_listen_sock != 0)
  {
    closesocket_compat(static_cast<socket_t>(m_listen_sock));
    m_listen_sock = 0;
  }
  if (m_thread.joinable())
    m_thread.join();
#ifdef _WIN32
  if (m_wsa_started)
  {
    WSACleanup();
    m_wsa_started = false;
  }
#endif
}

void Python_remote_server::accept_loop_()
{
  const socket_t listen_sock = static_cast<socket_t>(m_listen_sock);
  while (m_running.load())
  {
    sockaddr_in client_addr{};
#ifdef _WIN32
    int addr_len = sizeof(client_addr);
#else
    socklen_t addr_len = sizeof(client_addr);
#endif
    const socket_t client = ::accept(listen_sock, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
    if (client == k_invalid_socket)
    {
      if (!m_running.load())
        break;
      std::fprintf(stderr, "EzyCad: remote accept failed (err=%d)\n", last_socket_error());
      continue;
    }
    handle_client_(static_cast<uintptr_t>(client));
  }
}

void Python_remote_server::handle_client_(uintptr_t client_sock_u)
{
  const socket_t client = static_cast<socket_t>(client_sock_u);
  while (m_running.load())
  {
    std::string payload;
    if (!recv_frame(client, payload))
      break;

    int         req_id = 0;
    std::string code;
    try
    {
      const nlohmann::json j = nlohmann::json::parse(payload);
      req_id                 = j.value("id", 0);
      code                   = j.value("code", "");
    }
    catch (const std::exception& e)
    {
      nlohmann::json resp{{"id", req_id},
                          {"ok", false},
                          {"output", ""},
                          {"result", ""},
                          {"error", std::string("bad request JSON: ") + e.what()}};
      if (!send_frame(client, resp.dump()))
        break;
      continue;
    }

    auto                            promise = std::make_shared<std::promise<Python_exec_result>>();
    std::future<Python_exec_result> future  = promise->get_future();
    m_queue.enqueue(std::move(code), [promise](Python_exec_result r) { promise->set_value(std::move(r)); });

    Python_exec_result r;
    try
    {
      r = future.get();
    }
    catch (const std::exception& e)
    {
      r.ok    = false;
      r.error = std::string("execution queue failed: ") + e.what();
    }

    nlohmann::json resp{{"id", req_id}, {"ok", r.ok}, {"output", r.output}, {"result", r.result}, {"error", r.error}};
    if (!send_frame(client, resp.dump()))
      break;
  }
  closesocket_compat(client);
}

#else // !EZYCAD_HAVE_PYTHON

bool parse_listen_arg(const std::string& arg, Python_listen_endpoint& out, std::string& error)
{
  (void)arg;
  out   = Python_listen_endpoint{};
  error = "Python remote listen requires a build with EZYCAD_HAVE_PYTHON";
  return false;
}

void Python_execution_queue::enqueue(std::string code, Completer done)
{
  (void)code;
  if (done)
  {
    Python_exec_result r;
    r.ok    = false;
    r.error = "Python not available";
    done(std::move(r));
  }
}

void Python_execution_queue::process_pending(Python_console& console) { (void)console; }

Python_remote_server::Python_remote_server(Python_execution_queue& queue)
    : m_queue(queue)
{
}

Python_remote_server::~Python_remote_server() { stop(); }

bool Python_remote_server::start(const Python_listen_endpoint& ep, std::string& error)
{
  (void)ep;
  error = "Python remote listen requires a build with EZYCAD_HAVE_PYTHON";
  return false;
}

void Python_remote_server::stop() {}

void Python_remote_server::accept_loop_() {}

void Python_remote_server::handle_client_(uintptr_t) {}

#endif // EZYCAD_HAVE_PYTHON
