#pragma once

#include <string>
#include <string_view>

/// Returns a copy of \a s with leading and trailing ASCII whitespace removed.
std::string trim_copy(std::string s);

/// Returns a lowercased copy of \a s (ASCII only).
std::string to_lower_copy(std::string s);

/// Markdown-style heading anchor: lowercase, spaces to '-', drop other punctuation.
std::string slugify_heading(std::string s);

/// True when \a s begins with \a prefix.
bool str_starts_with(std::string_view s, std::string_view prefix);
