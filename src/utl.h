#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <vector>
#include <utility>

#include "utl_dbg.h"
#include "utl_types.h"

// ---------------------------------------------------------------------------
// Result / Status
// ---------------------------------------------------------------------------

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
      : m_v(status)
      , m_msg(msg)
  {
  }

  static Status ok(const std::string& msg = "") { return Status(Result_status::Success, msg); }

  static Status user_error(const std::string& msg) { return Status(Result_status::User_error, msg); }

  Result_status status() const noexcept { return m_v; }

  bool is_ok() const noexcept { return m_v == Result_status::Success; }

  const std::string& message() const noexcept { return m_msg; }

  bool operator==(const Result_status v) const noexcept { return status() == v; }

  Result_status m_v;
  std::string   m_msg;
};

template <typename T> class Result : public Status
{
public:
  Result();
  Result(const T& value);
  Result(T&& value);
  Result(const Result_status status, const std::string& error_msg = "");

  bool has_value() const noexcept;

  T&       value();
  const T& value() const;

  T&       operator*();
  const T& operator*() const;

private:
  std::optional<T> m_value;
};

/// Early return if not ok. Binds the expression once (safe for calls like `ensure_operation_shps_()`).
#define CHK_RET(expr)                                                                                                          \
  do                                                                                                                           \
  {                                                                                                                            \
    auto&& _ezy_chk_ret = (expr);                                                                                              \
    if (!_ezy_chk_ret.is_ok())                                                                                                 \
      return _ezy_chk_ret;                                                                                                     \
  } while (0)

/**
 * @brief Clears a variable number of arguments recursively.
 *
 * Supported types:
 * - Containers with `clear()` (e.g., `std::vector`, `std::string`)
 * - Arithmetic types (e.g., `int`, `float`), set to 0
 * - `std::optional`, set to `std::nullopt`
 *
 * Fails at compile time for unsupported types.
 */
template <typename T, typename... Args> void clear_all(T& arg, Args&... args);

// Templated function to pop and return the back element of a container
template <typename Container> typename Container::value_type&& pop_back(Container& container)
{
  EZY_ASSERT(container.size());
  auto value = std::move(container.back());
  container.pop_back();
  return std::move(value);
}

/*
 * Applies a lambda function to each non-container element in the provided arguments, flattening
 * containers by iterating their contents.
 */
template <typename Lambda, typename... Args> void for_each_flat(Lambda&& lambda, Args&&... args)
{
  (handle_arg(std::forward<Lambda>(lambda), std::forward<Args>(args)), ...);
}

/**
 * @brief Appends elements to a target container from a source.
 */
template <typename Container, typename T> void append(Container& target, const T& source);

struct Pair_hash
{
  std::size_t operator()(const std::pair<size_t, size_t>& p) const;
};

template <typename Container, typename Value> bool contains(const Container& container, const Value& value)
{
  auto it = std::find(container.begin(), container.end(), value);
  return it != container.end();
}

/// Returns \a base_name or \a base_name.NNN (three-digit suffix) not used in \a existing_names.
std::string unique_sequential_name(const std::string& base_name, std::span<const std::string> existing_names);

uint32_t load_texture(const std::string& path);

/// Decode PNG/JPEG/BMP/etc. from memory into RGBA8 (via stb_image). Empty if unsupported or corrupt.
std::optional<std::tuple<std::vector<uint8_t>, int, int>> decode_image_bytes(const std::string& file_bytes);

void disable_shape_highlighting(const AIS_Shape_ptr& ais_shape, const AIS_InteractiveContext_ptr& context,
                                bool disable_selection = false);

// Portable safe copy for fixed-size char buffers used with ImGui::InputText.
// Uses strncpy_s (with _TRUNCATE) on MSVC to avoid C4996; falls back elsewhere.
void safe_cstr_copy(char* dest, size_t dest_size, const char* src);

#include "utl.inl"
