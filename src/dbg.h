#pragma once

#include <filesystem>
#include <iostream>

// Platform-specific debug break
#ifdef _MSC_VER  // Windows (Visual Studio)
#include <intrin.h>
#define DEBUG_BREAK __debugbreak()
#else  // Unix-like (GDB, etc.)
#include <csignal>
#define DEBUG_BREAK raise(SIGTRAP)
#endif

#define EZY_ASSERT_MSG(condition, message)               \
  do                                                    \
  {                                                     \
    if (!(condition))                                   \
    {                                                   \
      std::cerr << "Assertion failed: " << #condition   \
                << ", message: " << (message)           \
                << ", file: " << __FILE__               \
                << ", line: " << __LINE__ << std::endl; \
      DEBUG_BREAK;                                      \
    }                                                   \
  } while (false)

#define EZY_ASSERT(condition)                            \
  do                                                    \
  {                                                     \
    if (!(condition))                                   \
    {                                                   \
      std::cerr << "Assertion failed: " << #condition   \
                << ", file: " << __FILE__               \
                << ", line: " << __LINE__ << std::endl; \
      DEBUG_BREAK;                                      \
    }                                                   \
  } while (false)

#define DBG_MSG(stream_expr)                                                                                                          \
  do                                                                                                                                  \
  {                                                                                                                                   \
    std::stringstream ss;                                                                                                             \
    ss << stream_expr;                                                                                                                \
    std::stringstream msg;                                                                                                            \
    msg << "[" << std::filesystem::path(__FILE__).filename().string() << ":" << __LINE__ << " " << __FUNCTION__ << " ] " << ss.str(); \
    std::cout << msg.str() << std::endl;                                                                                              \
  } while (0)