// Helper to fill vector (base case: no arguments)
template <typename T>
inline void fill_vector(std::vector<T>&) {}

// Helper to fill vector (recursive case)
template <typename T, typename... Args>
inline void fill_vector(std::vector<T>& vec, const T& first, Args... rest)
{
  static_assert((std::is_same_v<T, Args> && ...), "All arguments must be of the same type");
  vec.push_back(first);
  fill_vector(vec, rest...);
}

// Checks if elements are unique using IsEqual
template <typename T, typename... Args>
bool unique(const T& first, Args... args)
{
  // Collect arguments into vector
  std::vector<T> items;
  items.reserve(1 + sizeof...(args));
  fill_vector(items, first, args...);
  EZY_ASSERT(items.size() > 1);

  // Compare each pair
  for (size_t i = 0; i < items.size() - 1; ++i)
    for (size_t j = i + 1; j < items.size(); ++j)
      if (items[i].IsEqual(items[j], Precision::Confusion()))
        return false;

  return true;
}

// Checks if elements are equal using IsEqual
template <typename T, typename... Args>
bool equal(const T& first, Args... args)
{
  // Collect arguments into vector
  std::vector<T> items;
  items.reserve(1 + sizeof...(args));
  fill_vector(items, first, args...);
  EZY_ASSERT(items.size() > 1);

  // Compare each pair
  for (size_t i = 0; i < items.size() - 1; ++i)
    for (size_t j = i + 1; j < items.size(); ++j)
      if (!items[i].IsEqual(items[j], Precision::Confusion()))
        return false;

  return true;
}

// Convert degrees to radians
constexpr double to_radians(double degrees)
{
  return degrees * std::numbers::pi / 180.0;
}

// Convert radians to degrees
constexpr double to_degrees(double radians)
{
  return radians * 180.0 / std::numbers::pi;
}
