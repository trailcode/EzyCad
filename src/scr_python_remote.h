#pragma once

#include "scr_python_console.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/// Parsed `--listen [host:]port`. Default host is 127.0.0.1 when only a port is given.
struct Python_listen_endpoint
{
  std::string host = "127.0.0.1";
  uint16_t    port = 0;

  bool valid() const { return port != 0; }
};

/// Parse `8765` or `127.0.0.1:8765`. Returns false and sets error on failure.
bool parse_listen_arg(const std::string& arg, Python_listen_endpoint& out, std::string& error);

/// Thread-safe queue: remote listener enqueues; main thread runs process_pending().
class Python_execution_queue
{
public:
  using Completer = std::function<void(Python_exec_result)>;

  void enqueue(std::string code, Completer done);
  void process_pending(Python_console& console);

private:
  struct Job
  {
    std::string code;
    Completer   done;
  };

  std::mutex       m_mutex;
  std::vector<Job> m_jobs;
};

/// Background TCP server: length-prefixed UTF-8 JSON frames for remote Python console exec.
class Python_remote_server
{
public:
  explicit Python_remote_server(Python_execution_queue& queue);
  ~Python_remote_server();

  Python_remote_server(const Python_remote_server&)            = delete;
  Python_remote_server& operator=(const Python_remote_server&) = delete;

  bool start(const Python_listen_endpoint& ep, std::string& error);
  void stop();

private:
  void accept_loop_();
  void handle_client_(uintptr_t client_sock);

  Python_execution_queue& m_queue;
  Python_listen_endpoint  m_ep;
  std::thread             m_thread;
  std::atomic<bool>       m_running{false};
  uintptr_t               m_listen_sock = 0;
#ifdef _WIN32
  bool m_wsa_started = false;
#endif
};
