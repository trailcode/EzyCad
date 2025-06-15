#pragma once

#include <GLFW/glfw3.h>

#include <optional>

#include "dbg.h"
#include "types.h"

#define CHK_RET(status)  \
  do                     \
  {                      \
    if (!status.is_ok()) \
      return (status);   \
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
template <typename T, typename... Args>
void clear_all(T& arg, Args&... args);

// Templated function to pop and return the back element of a container
// Templated function to pop and return the back element of a container
template <typename Container>
typename Container::value_type&& pop_back(Container& container)
{
  DO_ASSERT(container.size());               // Ensure the container isn�t empty
  auto value = std::move(container.back());  // Get the last element
  container.pop_back();                      // Remove the last element
  return std::move(value);                   // Return the retrieved element
}

/*
 * Applies a lambda function to each non-container element in the provided arguments, flattening
 * containers by iterating their contents.
 * Non-lambda parameters (Args&&... args):
 *   - Thes e are a variadic pack of arguments that follow the lambda and can be any type:
 *     1. Non-container objects (e.g., int, double, custom structs, smart pointers like Handle<T>),
 *        which are passed directly to the lambda as-is.
 *     2. Containers (e.g., std::vector<T>, std::list<T>, arrays), which are iterable collections
 *        with begin() and end() methods. The function will iterate over their elements and apply
 *        the lambda to each one individually.
 *   - Multiple arguments of any mix of types can be passed in any order after the lambda. For
 *     non-containers, the lambda is called once per argument; for containers, it�s called once
 *     per element within the container.
 *   - The lambda�s parameter type must be compatible with the types of the non-container elements
 *     (or their common type if heterogeneous). No specific type is enforced on the arguments;
 *     type mismatches are caught at compile time by lambda deduction failure.
 * The lambda is the first parameter and defines how each flattened element is processed.
 */
template <typename Lambda, typename... Args>
void for_each_flat(Lambda&& lambda, Args&&... args)
{
  // Fold expression to process each argument
  (handle_arg(std::forward<Lambda>(lambda), std::forward<Args>(args)), ...);
}

/**
 * @brief Appends elements to a target container from a source.
 *
 * Supported source types:
 * - Single element matching `Container::value_type` (e.g., `int` for `std::vector<int>`)
 * - Container with the same `value_type` as the target (e.g., `std::vector<int>` to `std::vector<int>`)
 *
 * Fails at compile time for unsupported source types.
 */
template <typename Container, typename T>
void append(Container& target, const T& source);

// TODO find something more generic.
// Custom hash for std::pair<size_t, size_t>
struct Pair_hash
{
  std::size_t operator()(const std::pair<size_t, size_t>& p) const;
};

template <typename Container, typename Value>
bool contains(const Container& container, const Value& value)
{
  auto it = std::find(container.begin(), container.end(), value);
  return it != container.end();
}

GLuint load_texture(const std::string& path);

// Function to disable highlighting for a specific AIS_Shape
void disable_shape_highlighting(const AIS_Shape_ptr&              ais_shape,
                                const AIS_InteractiveContext_ptr& context,
                                Standard_Boolean                  disable_selection = Standard_False);

#include "utl.inl"