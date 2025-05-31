// Helper function for `for_each_flat`
template <typename Lambda, typename T>
void handle_arg(Lambda&& lambda, T&& arg)
{
  // Check if T is a container (has begin() and end())
  if constexpr (requires { arg.begin(); arg.end(); })
    for (auto&& elem : arg)
      lambda(std::forward<decltype(elem)>(elem));
  
  else
    lambda(std::forward<T>(arg));
}

// Trait to detect if a type has a clear() method
template <typename T, typename = void>
struct has_clear : std::false_type
{
};

template <typename T>
struct has_clear<T, std::void_t<decltype(std::declval<T>().clear())>> : std::true_type
{
};

template <typename T, typename = void>
struct has_reset : std::false_type
{
};

template <typename T>
struct has_reset<T, std::void_t<decltype(std::declval<T>().reset())>> : std::true_type
{
};

template <typename T, typename = void>
struct has_nullify : std::false_type
{
};

template <typename T>
struct has_nullify<T, std::void_t<decltype(std::declval<T>().Nullify())>> : std::true_type
{
};

// Helper trait to check if a type is std::optional
template <typename T>
struct is_optional : std::false_type
{
};

template <typename U>
struct is_optional<std::optional<U>> : std::true_type
{
};

template <typename T>
inline constexpr bool is_optional_v = is_optional<T>::value;

// Base case for recursion
inline void clear_all() {}

/**
 * @brief Clears a variable number of arguments recursively.
 *
 * Supported types:
 * - Containers with `clear()` (e.g., `std::vector`, `std::string`)
 * - Arithmetic types (e.g., `int`, `float`), set to 0
 * - `std::optional`, set to `std::nullopt`
 * - `opencascade::handle`, call Nullify()
 * - Raw pointers are set to nullptr
 *
 * Fails at compile time for unsupported types.
 */
template <typename T, typename... Args>
void clear_all(T& arg, Args&... args)
{
  if constexpr (std::is_pointer_v<T>)
    // Raw pointer: set to nullptr
    arg = nullptr;
  else if constexpr (has_clear<T>::value)
    // If T has a clear() method, call it
    arg.clear();
  else if constexpr (has_reset<T>::value)
    arg.reset();
  else if constexpr (std::is_arithmetic_v<T>)
    // If T is an arithmetic type (int, float, etc.), set to 0
    arg = 0;
  else if constexpr (is_optional_v<std::remove_cv_t<T>>)
    // If T is std::optional, set to std::nullopt
    arg = std::nullopt;
  else if constexpr (has_nullify<T>::value)
    arg.Nullify();
  else
    // Fail for unsupported types
    static_assert(false, "clear_all: Type must have clear(), be arithmetic, opencascade::handle, or be std::optional");
  
  // Recursively process remaining arguments
  clear_all(args...);
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
void append(Container& target, const T& source)
{
  // Extract the value_type of the target container
  using target_value_type = typename Container::value_type;

  // Case 1: T is the same as the container's value_type (single element)
  if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, target_value_type>)
    target.push_back(source);  // Append the single element
  // Case 2: T is a container with the same value_type
  else if constexpr (requires { source.begin(); source.end(); } &&
                     std::is_same_v<typename T::value_type, target_value_type>)
    target.insert(target.end(), source.begin(), source.end());  // Append all elements from source
  else
    static_assert(false, "Source must be either a single element of Container::value_type or a container with the same value_type");
}
