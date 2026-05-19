#include "utl_str.h"

#include <algorithm>
#include <cctype>

std::string trim_copy(std::string s)
{
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
    s.erase(s.begin());
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
    s.pop_back();
  return s;
}

std::string to_lower_copy(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

std::string slugify_heading(std::string s)
{
  s = to_lower_copy(trim_copy(std::move(s)));
  std::string out;
  out.reserve(s.size());
  bool prev_dash = false;
  for (unsigned char c : s)
  {
    if (std::isalnum(c))
    {
      out.push_back(static_cast<char>(c));
      prev_dash = false;
    }
    else if ((c == ' ' || c == '-' || c == '_') && !prev_dash)
    {
      out.push_back('-');
      prev_dash = true;
    }
  }
  while (!out.empty() && out.front() == '-')
    out.erase(out.begin());
  while (!out.empty() && out.back() == '-')
    out.pop_back();
  return out;
}

bool str_starts_with(const std::string_view s, const std::string_view prefix)
{
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}
