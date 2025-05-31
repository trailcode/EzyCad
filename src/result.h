#pragma once

#include <optional>
#include <stdexcept>
#include <string>

#include "dbg.h"

// Enum class for result status
enum class Result_status : uint8_t
{
  Success,
  Error,
  User_error,
  Topo_error,
  Null
};

class Status
{
 public:
  Status(const Result_status status, const std::string& msg = "")
      : m_v(status), m_msg(msg) {}

  static Status ok(const std::string& msg = "")
  {
    return Status(Result_status::Success, msg);
  }

  static Status user_error(const std::string& msg)
  {
    return Status(Result_status::User_error, msg);
  }

  // Get the status
  Result_status status() const noexcept
  {
    return m_v;
  }

  bool is_ok() const noexcept
  {
    return m_v == Result_status::Success;
  }

  // Get the error message (if any)
  const std::string& message() const noexcept
  {
    return m_msg;
  }

  bool operator==(const Result_status v) const noexcept
  {
    return status() == v;
  }

 //protected:
  Result_status m_v;
  std::string   m_msg;
};

// Templated Result class
template <typename T>
class Result : public Status
{
 public:
  // Default constructor sets status to Null
  Result();

  // Constructors for value
  Result(const T& value);
  Result(T&& value);

  // Constructor for error states
  Result(const Result_status status, const std::string& error_msg = "");

  // Check if the result contains a valid value
  bool has_value() const noexcept;

  // Get the value (throws if no value)
  T& value();

  const T& value() const;

  // Operator* to access the value
  T&       operator*();
  const T& operator*() const;

 private:
  std::optional<T> m_value;
};

#include "result.inl"