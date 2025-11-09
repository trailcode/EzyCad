
template <typename T>
Result<T>::Result()
    : Status(Result_status::Null), m_value(std::nullopt) {}

// Constructors for value
template <typename T>
Result<T>::Result(const T& value)
    : Status(Result_status::Success), m_value(value) {}

template <typename T>
Result<T>::Result(T&& value)
    : Status(Result_status::Success), m_value(std::move(value)) {}

// Constructor for error states
template <typename T>
Result<T>::Result(const Result_status status, const std::string& error_msg)
    : Status(status, error_msg), m_value(std::nullopt)
{
  EZY_ASSERT_MSG(status != Result_status::Success,
                "Success status requires a value");

  EZY_ASSERT_MSG(status != Result_status::Null || error_msg.empty(),
                "Null status should not have an error message");
}

// Check if the result contains a valid value
template <typename T>
bool Result<T>::has_value() const noexcept
{
  return m_v == Result_status::Success && m_value.has_value();
}

// Get the value (throws if no value)
template <typename T>
T& Result<T>::value()
{
  if (!has_value())
  {
    EZY_ASSERT_MSG(m_v != Result_status::Null,
                  "Result is in Null state");

    EZY_ASSERT_MSG(false, m_msg.empty() ? "No value available" : m_msg);
  }
  return *m_value;
}

template <typename T>
const T& Result<T>::value() const
{
  if (!has_value())
  {
    EZY_ASSERT_MSG(m_v != Result_status::Null,
                  "Result is in Null state");

    EZY_ASSERT_MSG(false, m_msg.empty() ? "No value available" : m_msg);
  }
  return *m_value;
}

// Operator* to access the value
template <typename T>
T& Result<T>::operator*()
{
  return value();
}

template <typename T>
const T& Result<T>::operator*() const
{
  return value();
}
