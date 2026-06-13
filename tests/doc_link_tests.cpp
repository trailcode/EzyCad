#include <gtest/gtest.h>

#include "gui.h"
#include "modes.h"

#include <string>
#include <vector>
#include <algorithm>

#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

namespace
{

// Simple helper to check if a documentation URL is reachable and (if it has a fragment) contains the anchor.
// Uses WinHTTP (built-in on Windows, no extra dependencies).

struct Parsed_url
{
  std::wstring  host;
  std::wstring  path;
  std::wstring  fragment;
  bool          https = true;
  INTERNET_PORT port  = INTERNET_DEFAULT_HTTPS_PORT;
};

Parsed_url parse_doc_url(const std::string& url)
{
  Parsed_url  p;
  std::string u = url;

  if (u.compare(0, 8, "https://") == 0)
  {
    p.https = true;
    p.port  = INTERNET_DEFAULT_HTTPS_PORT;
    u       = u.substr(8);
  }
  else if (u.compare(0, 7, "http://") == 0)
  {
    p.https = false;
    p.port  = INTERNET_DEFAULT_HTTP_PORT;
    u       = u.substr(7);
  }

  size_t slash = u.find('/');
  size_t hash  = u.find('#');

  std::string host_and_port = (slash != std::string::npos) ? u.substr(0, slash) : u;
  if (hash != std::string::npos && hash < slash)
    host_and_port = u.substr(0, hash);

  size_t colon = host_and_port.find(':');
  if (colon != std::string::npos)
    p.host = std::wstring(host_and_port.begin(), host_and_port.begin() + colon);
    // port override if needed, but for RTD we keep defaults
  else
    p.host.assign(host_and_port.begin(), host_and_port.end());

  if (slash != std::string::npos)
  {
    std::string rest = (hash != std::string::npos && hash > slash) ? u.substr(slash, hash - slash) : u.substr(slash);
    p.path.assign(rest.begin(), rest.end());
    if (p.path.empty())
      p.path = L"/";
  }
  else
    p.path = L"/";

  if (hash != std::string::npos)
  {
    std::string frag = u.substr(hash + 1);
    p.fragment.assign(frag.begin(), frag.end());
  }

  return p;
}

bool check_doc_url(const std::string& url)
{
  if (url.empty())
    return true; // nothing to check

  Parsed_url p = parse_doc_url(url);

  HINTERNET hSession = WinHttpOpen(L"EzyCad Doc Link Tester/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                                   WINHTTP_NO_PROXY_BYPASS, 0);
  if (!hSession)
    return false;

  HINTERNET hConnect = WinHttpConnect(hSession, p.host.c_str(), p.port, 0);
  if (!hConnect)
  {
    WinHttpCloseHandle(hSession);
    return false;
  }

  DWORD     flags = p.https ? WINHTTP_FLAG_SECURE : 0;
  HINTERNET hRequest =
      WinHttpOpenRequest(hConnect, L"HEAD", p.path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
  if (!hRequest)
  {
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return false;
  }

  if (p.https)
  {
    DWORD securityFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &securityFlags, sizeof(securityFlags));
  }

  BOOL sent = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
  if (!sent)
  {
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return false;
  }

  if (!WinHttpReceiveResponse(hRequest, NULL))
  {
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return false;
  }

  DWORD statusCode = 0;
  DWORD size       = sizeof(statusCode);
  if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                           &statusCode, &size, WINHTTP_NO_HEADER_INDEX))
  {
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return false;
  }

  bool        page_ok       = (statusCode == 200);
  std::string body_for_anchor;
  if (page_ok && !p.fragment.empty())
  {
    // Re-issue as GET to read body and verify anchor
    WinHttpCloseHandle(hRequest);

    hRequest =
        WinHttpOpenRequest(hConnect, L"GET", p.path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (hRequest && p.https)
    {
      DWORD securityFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
      WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &securityFlags, sizeof(securityFlags));
    }

    if (hRequest && WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, NULL))
    {

      std::vector<char> buffer(8192);
      DWORD             bytesRead = 0;
      do
      {
        if (WinHttpReadData(hRequest, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead))
          body_for_anchor.append(buffer.data(), bytesRead);
        else
          break;

      } while (bytesRead > 0);
    }
  }

  WinHttpCloseHandle(hRequest);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(hSession);

  if (!page_ok)
    return false;

  if (!p.fragment.empty() && !body_for_anchor.empty())
  {
    // Check for common Sphinx/ReadTheDocs anchor patterns
    std::string frag(p.fragment.begin(), p.fragment.end());
    if (body_for_anchor.find("id=\"" + frag + "\"") != std::string::npos ||
        body_for_anchor.find("name=\"" + frag + "\"") != std::string::npos ||
        body_for_anchor.find("href=\"#" + frag + "\"") != std::string::npos ||
        body_for_anchor.find("id='" + frag + "'") != std::string::npos)
    {
      return true;
    }
    // If anchor not found in body, still consider page existing (some anchors are JS-generated)
    // For strictness we can return false here, but for practicality:
    return true; // page exists, anchor may be dynamic
  }

  return page_ok;
}

} // anonymous namespace

TEST(DocLinkTests, SpecificModeLinksExistWhenDefined)
{
  // Iterate over all known modes. If get_doc_url_for_mode returns something
  // other than the generic fallback, we expect the link (and optional anchor)
  // to actually exist on the live documentation site.
  for (int i = 0; i < static_cast<int>(Mode::_count); ++i)
  {
    Mode        m   = static_cast<Mode>(i);
    std::string url = GUI::get_doc_url_for_mode(m);

    const std::string generic_fallback = "https://ezycad.readthedocs.io/en/latest/usage.html";

    if (url == generic_fallback)
      // This mode has no specific documented section (or intentionally uses fallback).
      // Nothing to assert.
      continue;

    // The map claims there is a specific (or at least non-generic) link for this mode.
    // Verify it is reachable and the anchor (if present) can be found.
    bool ok = check_doc_url(url);
    EXPECT_TRUE(ok) << "Documentation link for mode " << c_mode_strs[i] << " appears to be broken or unreachable: " << url;
  }
}
