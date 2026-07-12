#pragma once

#include "scr_python_console.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
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
  /// Fail all pending jobs and reject new ones (unblocks remote waiters on shutdown).
  void shutdown();

private:
  struct Job
  {
    std::string code;
    Completer   done;
  };

  std::mutex       m_mutex;
  std::vector<Job> m_jobs;
  bool             m_shutdown = false;
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
  void track_client_(uintptr_t client_sock);
  void untrack_client_(uintptr_t client_sock);
  void close_all_clients_();

  Python_execution_queue&       m_queue;
  Python_listen_endpoint        m_ep;
  std::thread                   m_thread;
  std::atomic<bool>             m_running{false};
  uintptr_t                     m_listen_sock = 0;
  std::mutex                    m_clients_mutex;
  std::unordered_set<uintptr_t> m_clients;
#ifdef _WIN32
  bool m_wsa_started = false;
#endif
};
