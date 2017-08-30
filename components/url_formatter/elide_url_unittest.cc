// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/url_formatter/elide_url.h"

#include <stddef.h>

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/url_formatter/url_formatter.h"
#include "net/base/escape.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

#if !defined(OS_ANDROID)
#include "ui/gfx/font_list.h"  // nogncheck
#include "ui/gfx/text_elider.h"  // nogncheck
#include "ui/gfx/text_utils.h"  // nogncheck
#endif

namespace {

struct Testcase {
  const std::string input;
  const std::string output;
};

struct Component {
  url::Parsed::ComponentType type;
  int begin;
  int len;
};

struct ParsedTestcase {
  const std::string input;
  const std::string output;
  const std::vector<Component> components;
};

void RunElisionTest(const std::vector<Testcase>& testcases,
                    bool preserve_filename) {
  const gfx::FontList font_list;
  for (const auto& testcase : testcases) {
    SCOPED_TRACE(testcase.input + " to " + testcase.output);

    const GURL url(testcase.input);
    const float available_width =
        gfx::GetStringWidthF(base::UTF8ToUTF16(testcase.output), font_list);
    base::string16 elided;
    if (preserve_filename) {
      elided = url_formatter::ElideUrl(url, font_list, available_width);
    } else {
      url::Parsed parsed;
      elided = url_formatter::SimpleElideUrl(url, font_list, available_width,
                                             &parsed);
    }
    EXPECT_EQ(base::UTF8ToUTF16(testcase.output), elided);
  }
}

// Test eliding of commonplace URLs.
TEST(TextEliderTest, TestGeneralEliding) {
  const std::string kEllipsisStr(gfx::kEllipsis);
  const std::vector<Testcase> testcases = {
      // HTTP with directoy path
      {"http://www.google.com/intl/en/ads/", "www.google.com/intl/en/ads/"},
      {"http://www.google.com/intl/en/ads/",
       "www.google.com/intl/" + kEllipsisStr + "/ads/"},
      {"http://www.google.com/intl/en/ads/",
       "www.google.com/" + kEllipsisStr + "/ads/"},
      {"http://www.google.com/intl/en/ads/", "www.google.com/" + kEllipsisStr},
      {"http://www.google.com/intl/en/ads/",
       kEllipsisStr + "google.com/" + kEllipsisStr},

      // HTTP with file path
      {"http://www.google.com/intl/en/ads.html",
       "www.google.com/intl/en/ads.html"},
      {"http://www.google.com/intl/en/ads.html",
       "www.google.com/intl/" + kEllipsisStr + "/ads.html"},
      {"http://www.google.com/intl/en/ads.html",
       "www.google.com/" + kEllipsisStr + "/ads.html"},
      {"http://www.google.com/intl/en/ads.html",
       "www.google.com/" + kEllipsisStr},

      // HTTP without path
      {"http://www.google.com/", "www.google.com"},
      {"http://www.google.com/", kEllipsisStr + "w.google.com"},
      {"http://www.google.com/", kEllipsisStr + "gle.com"},

      // HTTPS with path
      {"https://subdomain.foo.com/bar/filename.html",
       "https://subdomain.foo.com/bar/filename.html"},
      {"https://subdomain.foo.com/bar/filename.html",
       "https://subdomain.foo.com/" + kEllipsisStr},
      {"https://subdomain.foo.com/bar/filename.html",
       "subdomain.foo.com/" + kEllipsisStr},
      {"https://subdomain.foo.com/bar/filename.html",
       kEllipsisStr + "n.foo.com/" + kEllipsisStr},

      // HTTP with slashes in path.
      {"http://www.google.com/////////////",
       "www.google.com/" + kEllipsisStr + "//"},

      // Original cases. Adapt these to the new approach as needed.
      {"https://www.google.com/intl/en/ads/",
       "https://www.google.com/intl/en/ads/"},
      {"https://www.url.com/sub/directory/file/",
       "https://www.url.com/sub/directory/file/"},
      {"https://www.url.com/sub/directory/file/",
       "https://www.url.com/sub/" + kEllipsisStr + "/file/"},
      {"https://www.url.com/sub/directory/file/",
       "https://www.url.com/" + kEllipsisStr + "/file/"},
      {"https://www.url.com/sub/directory/file/",
       "https://www.url.com/" + kEllipsisStr},
      {"https://www.url.com/sub/directory/file/",
       "www.url.com/" + kEllipsisStr},
      {"https://www.url.com/sub/directory/file/",
       kEllipsisStr + "url.com/" + kEllipsisStr},
      {"https://www.url.com/sub/directory/file/?aLongQueryWhichIsNotRequired",
       "https://www.url.com/sub/directory/file/?aLongQuery" + kEllipsisStr},
      {"https://www.url.com/sub/directory/file/?aLongQueryWhichIsNotRequired",
       "https://www.url.com/" + kEllipsisStr + "/file/" + kEllipsisStr},
      {"http://subdomain.foo.com/bar/filename.html",
       "subdomain.foo.com/" + kEllipsisStr + "/filename.html"},
      {"http://www.google.com/intl/en/ads/?aLongQueryWhichIsNotRequired",
       "www.google.com/intl/en/ads/?aLongQ" + kEllipsisStr},
  };

  RunElisionTest(testcases, true);
}

// TODO(cjgrant): This should not be needed anymore, but make sure it works.

// When there is very little space available, the elision code will shorten
// both path AND file name to an ellipsis - ".../...". To avoid this result,
// there is a hack in place that simply treats them as one string in this
// case.
#if 0
TEST(TextEliderTest, TestTrailingEllipsisSlashEllipsisHack) {
  const std::string kEllipsisStr(gfx::kEllipsis);

  // Very little space, would cause double ellipsis.
  gfx::FontList font_list;
  GURL url("http://battersbox.com/directory/foo/peter_paul_and_mary.html");
  float available_width = gfx::GetStringWidthF(
      base::UTF8ToUTF16("battersbox.com/" + kEllipsisStr + "/" + kEllipsisStr),
      font_list);

  // Create the expected string, after elision. Depending on font size, the
  // directory might become /dir... or /di... or/d... - it never should be
  // shorter than that. (If it is, the font considers d... to be longer
  // than .../... -  that should never happen).
  ASSERT_GT(
      gfx::GetStringWidthF(base::UTF8ToUTF16(kEllipsisStr + "/" + kEllipsisStr),
                           font_list),
      gfx::GetStringWidthF(base::UTF8ToUTF16("d" + kEllipsisStr), font_list));
  GURL long_url("http://battersbox.com/directorynameisreallylongtoforcetrunc");
  base::string16 expected = url_formatter::ElideUrl(
      long_url, font_list, available_width);
  // Ensure that the expected result still contains part of the directory name.
  ASSERT_GT(expected.length(), std::string("battersbox.com/d").length());
  EXPECT_EQ(expected, url_formatter::ElideUrl(url, font_list, available_width));

  // More space available - elide directories, partially elide filename.
  const std::vector<Testcase> testcases = {
      {"http://battersbox.com/directory/foo/peter_paul_and_mary.html",
       "battersbox.com/" + kEllipsisStr + "/peter" + kEllipsisStr},
  };
  RunElisionTest(testcases, true);
}
#endif

// Test eliding of empty strings, URLs with ports, passwords, queries, etc.
TEST(TextEliderTest, TestMoreEliding) {
#if defined(OS_WIN)
  // Needed to bypass DCHECK in GetFallbackFont.
  base::MessageLoopForUI message_loop;
#endif
  const std::string kEllipsisStr(gfx::kEllipsis);
  const std::vector<Testcase> testcases = {
      // Eliding the same URL to various lengths.
      {"http://www.google.com/foo?bar", "www.google.com/foo?bar"},
      {"http://xyz.google.com/foo?bar", "xyz.google.com/foo?" + kEllipsisStr},
      {"http://xyz.google.com/foo?bar", "xyz.google.com/foo" + kEllipsisStr},
      {"http://xyz.google.com/foo?bar", "xyz.google.com/fo" + kEllipsisStr},
      {"http://xyz.google.com/foo?bar", "xyz.google.com/" + kEllipsisStr},
      {"http://xyz.google.com/foo?bar",
       kEllipsisStr + "z.google.com/" + kEllipsisStr},
      {"http://xyz.google.com/foo?bar",
       kEllipsisStr + ".google.com/" + kEllipsisStr},
      {"http://xyz.google.com/foo?bar",
       kEllipsisStr + "google.com/" + kEllipsisStr},
      {"http://xyz.google.com/foo?bar",
       kEllipsisStr + "oogle.com/" + kEllipsisStr},
      {"http://xyz.google.com/foo?bar", kEllipsisStr + "e.com/" + kEllipsisStr},
      {"http://xyz.google.com/foo?bar", kEllipsisStr + "com/" + kEllipsisStr},

      // TODO(cjgrant): Resolve the bug below.
      // URL with no path.
      // TODO(mgiuca): These should elide the start of the URL, not the end.
      // https://crbug.com/739636.
      {"http://xyz.google.com", kEllipsisStr + "oogle.com"},
      {"https://xyz.google.com", kEllipsisStr + "oogle.com"},

      {"http://a.b.com/pathname/c?d", "a.b.com/" + kEllipsisStr + "/c?d"},
      {"", ""},
      {"http://foo.bar..example.com...hello/test/filename.html",
       "foo.bar..example.com...hello/" + kEllipsisStr + "/filename.html"},
      {"http://foo.bar../", "foo.bar.."},
      {"http://xn--1lq90i.cn/foo", "\xe5\x8c\x97\xe4\xba\xac.cn/foo"},
      {"http://me:mypass@secrethost.com:99/foo?bar#baz",
       "secrethost.com:99/foo?bar#baz"},
      {"http://me:mypass@ss%xxfdsf.com/foo", "ss%25xxfdsf.com/foo"},
      {"mailto:elgoato@elgoato.com", "mailto:elgoato@elgoato.com"},
      {"javascript:click(0)", "javascript:click(0)"},
      {"https://chess.eecs.berkeley.edu:4430/login/arbitfilename",
       "chess.eecs.berkeley.edu:4430/login/arbitfilename"},
      {"https://chess.eecs.berkeley.edu:4430/login/arbitfilename",
       kEllipsisStr + "berkeley.edu:4430/" + kEllipsisStr + "/arbitfilename"},

      // Unescaping.
      {"http://www/%E4%BD%A0%E5%A5%BD?q=%E4%BD%A0%E5%A5%BD#\xe4\xbd\xa0",
       "www/\xe4\xbd\xa0\xe5\xa5\xbd?q=\xe4\xbd\xa0\xe5\xa5\xbd#\xe4\xbd\xa0"},

      // Invalid unescaping for path. The ref will always be valid UTF-8. We
      // don't bother to do too many edge cases, since these are handled by the
      // escaper unittest.
      {"http://www/%E4%A0%E5%A5%BD?q=%E4%BD%A0%E5%A5%BD#\xe4\xbd\xa0",
       "www/%E4%A0%E5%A5%BD?q=\xe4\xbd\xa0\xe5\xa5\xbd#\xe4\xbd\xa0"},
  };

  RunElisionTest(testcases, false);
}

// TODO(cjgrant): Fix these too.
#if 0
// Test eliding of file: URLs.
TEST(TextEliderTest, TestFileURLEliding) {
  const std::string kEllipsisStr(gfx::kEllipsis);
  Testcase testcases[] = {
    {"file:///C:/path1/path2/path3/filename",
     "file:///C:/path1/path2/path3/filename"},
    {"file:///C:/path1/path2/path3/filename", "C:/path1/path2/path3/filename"},
// GURL parses "file:///C:path" differently on windows than it does on posix.
#if defined(OS_WIN)
    {"file:///C:path1/path2/path3/filename",
     "C:/path1/path2/" + kEllipsisStr + "/filename"},
    {"file:///C:path1/path2/path3/filename",
     "C:/path1/" + kEllipsisStr + "/filename"},
    {"file:///C:path1/path2/path3/filename",
     "C:/" + kEllipsisStr + "/filename"},
#endif  // defined(OS_WIN)
    {"file://filer/foo/bar/file", "filer/foo/bar/file"},
    {"file://filer/foo/bar/file", "filer/foo/" + kEllipsisStr + "/file"},
    {"file://filer/foo/bar/file", "filer/" + kEllipsisStr + "/file"},
    {"file://filer/foo/", "file://filer/foo/"},
    {"file://filer/foo/", "filer/foo/"},
    {"file://filer/foo/", "filer" + kEllipsisStr},
    // Eliding file URLs with nothing after the ':' shouldn't crash.
    {"file:///aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa:", "aaa" + kEllipsisStr},
    {"file:///aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa:/", "aaa" + kEllipsisStr},
  };

  RunUrlTest(testcases, arraysize(testcases));
}
#endif

TEST(TextEliderTest, TestHostEliding) {
  const std::string kEllipsisStr(gfx::kEllipsis);
  Testcase testcases[] = {
    {"http://google.com", "google.com"},
    {"http://reallyreallyreallylongdomainname.com",
     "reallyreallyreallylongdomainname.com"},
    {"http://foo", "foo"},
    {"http://foo.bar", "foo.bar"},
    {"http://subdomain.google.com", kEllipsisStr + ".google.com"},
    {"http://a.b.c.d.e.f.com", kEllipsisStr + "f.com"},
    {"http://subdomain.foo.bar", kEllipsisStr + "in.foo.bar"},
    {"http://subdomain.reallylongdomainname.com",
     kEllipsisStr + "ain.reallylongdomainname.com"},
    {"http://a.b.c.d.e.f.com", kEllipsisStr + ".e.f.com"},
    // IDN - Greek alpha.beta.gamma.delta.epsilon.zeta.com
    {"http://xn--mxa.xn--nxa.xn--oxa.xn--pxa.xn--qxa.xn--rxa.com",
     kEllipsisStr + ".\xCE\xB5.\xCE\xB6.com"},
  };

  for (size_t i = 0; i < arraysize(testcases); ++i) {
    const float available_width = gfx::GetStringWidthF(
        base::UTF8ToUTF16(testcases[i].output), gfx::FontList());
    EXPECT_EQ(base::UTF8ToUTF16(testcases[i].output),
              url_formatter::ElideHost(GURL(testcases[i].input),
                                       gfx::FontList(), available_width));
  }

  // Trying to elide to a really short length will still keep the full TLD+1
  EXPECT_EQ(
      base::ASCIIToUTF16("google.com"),
      url_formatter::ElideHost(GURL("http://google.com"), gfx::FontList(), 2));
  EXPECT_EQ(base::UTF8ToUTF16(kEllipsisStr + ".google.com"),
            url_formatter::ElideHost(GURL("http://subdomain.google.com"),
                                     gfx::FontList(), 2));
  EXPECT_EQ(
      base::ASCIIToUTF16("foo.bar"),
      url_formatter::ElideHost(GURL("http://foo.bar"), gfx::FontList(), 2));
}

struct OriginTestData {
  const char* const description;
  const char* const input;
  const wchar_t* const output;
  const wchar_t* const output_omit_web_scheme;
  const wchar_t* const output_omit_cryptographic_scheme;
};

// Common test data for both FormatUrlForSecurityDisplay() and
// FormatOriginForSecurityDisplay()
const OriginTestData common_tests[] = {
    {"Empty URL", "", L"", L"", L""},
    {"HTTP URL", "http://www.google.com/", L"http://www.google.com",
     L"www.google.com", L"http://www.google.com"},
    {"HTTPS URL", "https://www.google.com/", L"https://www.google.com",
     L"www.google.com", L"www.google.com"},
    {"Standard HTTP port", "http://www.google.com:80/",
     L"http://www.google.com", L"www.google.com", L"http://www.google.com"},
    {"Standard HTTPS port", "https://www.google.com:443/",
     L"https://www.google.com", L"www.google.com", L"www.google.com"},
    {"Standard HTTP port, IDN Chinese",
     "http://\xe4\xb8\xad\xe5\x9b\xbd.icom.museum:80",
     L"http://\x4e2d\x56fd.icom.museum", L"\x4e2d\x56fd.icom.museum",
     L"http://\x4e2d\x56fd.icom.museum"},
    {"HTTP URL, IDN Hebrew (RTL)",
     "http://"
     "\xd7\x90\xd7\x99\xd7\xa7\xd7\x95\xd7\xb4\xd7\x9d."
     "\xd7\x99\xd7\xa9\xd7\xa8\xd7\x90\xd7\x9c.museum/",
     L"http://xn--4dbklr2c8d.xn--4dbrk0ce.museum",
     L"xn--4dbklr2c8d.xn--4dbrk0ce.museum",
     L"http://xn--4dbklr2c8d.xn--4dbrk0ce.museum"},
    {"HTTP URL with query string, IDN Arabic (RTL)",
     "http://\xd9\x85\xd8\xb5\xd8\xb1.icom.museum/foo.html?yes=no",
     L"http://xn--wgbh1c.icom.museum", L"xn--wgbh1c.icom.museum",
     L"http://xn--wgbh1c.icom.museum"},
    {"Non-standard HTTP port", "http://www.google.com:9000/",
     L"http://www.google.com:9000", L"www.google.com:9000",
     L"http://www.google.com:9000"},
    {"Non-standard HTTPS port", "https://www.google.com:9000/",
     L"https://www.google.com:9000", L"www.google.com:9000",
     L"www.google.com:9000"},
    {"HTTP URL with path", "http://www.google.com/test.html",
     L"http://www.google.com", L"www.google.com", L"http://www.google.com"},
    {"HTTPS URL with path", "https://www.google.com/test.html",
     L"https://www.google.com", L"www.google.com", L"www.google.com"},
    {"Unusual secure scheme (wss)", "wss://www.google.com/",
     L"wss://www.google.com", L"wss://www.google.com", L"www.google.com"},
    {"Unusual non-secure scheme (gopher)", "gopher://www.google.com/",
     L"gopher://www.google.com", L"gopher://www.google.com",
     L"gopher://www.google.com"},
    {"Unlisted scheme (chrome)", "chrome://version", L"chrome://version",
     L"chrome://version", L"chrome://version"},
    {"HTTP IP address", "http://173.194.65.103", L"http://173.194.65.103",
     L"173.194.65.103", L"http://173.194.65.103"},
    {"HTTPS IP address", "https://173.194.65.103", L"https://173.194.65.103",
     L"173.194.65.103", L"173.194.65.103"},
    {"HTTP IPv6 address", "http://[FE80:0000:0000:0000:0202:B3FF:FE1E:8329]/",
     L"http://[fe80::202:b3ff:fe1e:8329]", L"[fe80::202:b3ff:fe1e:8329]",
     L"http://[fe80::202:b3ff:fe1e:8329]"},
    {"HTTPs IPv6 address", "https://[FE80:0000:0000:0000:0202:B3FF:FE1E:8329]/",
     L"https://[fe80::202:b3ff:fe1e:8329]", L"[fe80::202:b3ff:fe1e:8329]",
     L"[fe80::202:b3ff:fe1e:8329]"},
    {"HTTP IPv6 address with port",
     "http://[FE80:0000:0000:0000:0202:B3FF:FE1E:8329]:80/",
     L"http://[fe80::202:b3ff:fe1e:8329]", L"[fe80::202:b3ff:fe1e:8329]",
     L"http://[fe80::202:b3ff:fe1e:8329]"},
    {"HTTPs IPv6 address with port",
     "https://[FE80:0000:0000:0000:0202:B3FF:FE1E:8329]:443/",
     L"https://[fe80::202:b3ff:fe1e:8329]", L"[fe80::202:b3ff:fe1e:8329]",
     L"[fe80::202:b3ff:fe1e:8329]"},
    {"HTTPS IP address, non-default port", "https://173.194.65.103:8443",
     L"https://173.194.65.103:8443", L"173.194.65.103:8443",
     L"173.194.65.103:8443"},
    {"Invalid host 1", "https://www.cyber../wow.php", L"https://www.cyber..",
     L"www.cyber..", L"www.cyber.."},
    {"Invalid host 2", "https://www...cyber/wow.php", L"https://www...cyber",
     L"www...cyber", L"www...cyber"},
    {"Invalid port 3", "https://173.194.65.103:/hello.aspx",
     L"https://173.194.65.103", L"173.194.65.103", L"173.194.65.103"},
    {"Trailing dot in DNS name", "https://www.example.com./get/goat",
     L"https://www.example.com.", L"www.example.com.", L"www.example.com."}};

TEST(TextEliderTest, FormatUrlForSecurityDisplay) {
  for (size_t i = 0; i < arraysize(common_tests); ++i) {
    base::string16 formatted =
        url_formatter::FormatUrlForSecurityDisplay(GURL(common_tests[i].input));
    EXPECT_EQ(base::WideToUTF16(common_tests[i].output), formatted)
        << common_tests[i].description;

    base::string16 formatted_omit_web_scheme =
        url_formatter::FormatUrlForSecurityDisplay(
            GURL(common_tests[i].input),
            url_formatter::SchemeDisplay::OMIT_HTTP_AND_HTTPS);
    EXPECT_EQ(base::WideToUTF16(common_tests[i].output_omit_web_scheme),
              formatted_omit_web_scheme)
        << common_tests[i].description;

    base::string16 formatted_omit_cryptographic_scheme =
        url_formatter::FormatUrlForSecurityDisplay(
            GURL(common_tests[i].input),
            url_formatter::SchemeDisplay::OMIT_CRYPTOGRAPHIC);
    EXPECT_EQ(
        base::WideToUTF16(common_tests[i].output_omit_cryptographic_scheme),
        formatted_omit_cryptographic_scheme)
        << common_tests[i].description;
  }

  const OriginTestData tests[] = {
      {"File URI", "file:///usr/example/file.html",
       L"file:///usr/example/file.html", L"file:///usr/example/file.html",
       L"file:///usr/example/file.html"},
      {"File URI with hostname", "file://localhost/usr/example/file.html",
       L"file:///usr/example/file.html", L"file:///usr/example/file.html",
       L"file:///usr/example/file.html"},
      {"UNC File URI 1", "file:///CONTOSO/accounting/money.xls",
       L"file:///CONTOSO/accounting/money.xls",
       L"file:///CONTOSO/accounting/money.xls",
       L"file:///CONTOSO/accounting/money.xls"},
      {"UNC File URI 2",
       "file:///C:/Program%20Files/Music/Web%20Sys/main.html?REQUEST=RADIO",
       L"file:///C:/Program%20Files/Music/Web%20Sys/main.html",
       L"file:///C:/Program%20Files/Music/Web%20Sys/main.html",
       L"file:///C:/Program%20Files/Music/Web%20Sys/main.html"},
      {"Invalid IPv6 address", "https://[2001:db8:0:1]/",
       L"https://[2001:db8:0:1]", L"https://[2001:db8:0:1]",
       L"https://[2001:db8:0:1]"},
      {"HTTP filesystem: URL with path",
       "filesystem:http://www.google.com/temporary/test.html",
       L"filesystem:http://www.google.com", L"filesystem:http://www.google.com",
       L"filesystem:http://www.google.com"},
      {"File filesystem: URL with path",
       "filesystem:file://localhost/temporary/stuff/"
       "test.html?z=fun&goat=billy",
       L"filesystem:file:///temporary/stuff/test.html",
       L"filesystem:file:///temporary/stuff/test.html",
       L"filesystem:file:///temporary/stuff/test.html"},
      {"Invalid scheme 1", "twelve://www.cyber.org/wow.php",
       L"twelve://www.cyber.org/wow.php", L"twelve://www.cyber.org/wow.php",
       L"twelve://www.cyber.org/wow.php"},
      {"Invalid scheme 2", "://www.cyber.org/wow.php",
       L"://www.cyber.org/wow.php", L"://www.cyber.org/wow.php",
       L"://www.cyber.org/wow.php"},
      {"Invalid port 1", "https://173.194.65.103:000",
       L"https://173.194.65.103:0", L"173.194.65.103:0", L"173.194.65.103:0"},
      {"Invalid port 2", "https://173.194.65.103:gruffle",
       L"https://173.194.65.103:gruffle", L"https://173.194.65.103:gruffle",
       L"https://173.194.65.103:gruffle"},
      {"Blob URL",
       "blob:http://www.html5rocks.com/4d4ff040-6d61-4446-86d3-13ca07ec9ab9",
       L"blob:http://www.html5rocks.com/"
       L"4d4ff040-6d61-4446-86d3-13ca07ec9ab9",
       L"blob:http://www.html5rocks.com/"
       L"4d4ff040-6d61-4446-86d3-13ca07ec9ab9",
       L"blob:http://www.html5rocks.com/"
       L"4d4ff040-6d61-4446-86d3-13ca07ec9ab9"}};

  for (size_t i = 0; i < arraysize(tests); ++i) {
    base::string16 formatted =
        url_formatter::FormatUrlForSecurityDisplay(GURL(tests[i].input));
    EXPECT_EQ(base::WideToUTF16(tests[i].output), formatted)
        << tests[i].description;

    base::string16 formatted_omit_web_scheme =
        url_formatter::FormatUrlForSecurityDisplay(
            GURL(tests[i].input),
            url_formatter::SchemeDisplay::OMIT_HTTP_AND_HTTPS);
    EXPECT_EQ(base::WideToUTF16(tests[i].output_omit_web_scheme),
              formatted_omit_web_scheme)
        << tests[i].description;

    base::string16 formatted_omit_cryptographic_scheme =
        url_formatter::FormatUrlForSecurityDisplay(
            GURL(tests[i].input),
            url_formatter::SchemeDisplay::OMIT_CRYPTOGRAPHIC);
    EXPECT_EQ(base::WideToUTF16(tests[i].output_omit_cryptographic_scheme),
              formatted_omit_cryptographic_scheme)
        << tests[i].description;
  }

  base::string16 formatted = url_formatter::FormatUrlForSecurityDisplay(GURL());
  EXPECT_EQ(base::string16(), formatted)
      << "Explicitly test the 0-argument GURL constructor";

  base::string16 formatted_omit_scheme =
      url_formatter::FormatUrlForSecurityDisplay(
          GURL(), url_formatter::SchemeDisplay::OMIT_HTTP_AND_HTTPS);
  EXPECT_EQ(base::string16(), formatted_omit_scheme)
      << "Explicitly test the 0-argument GURL constructor";

  formatted_omit_scheme = url_formatter::FormatUrlForSecurityDisplay(
      GURL(), url_formatter::SchemeDisplay::OMIT_CRYPTOGRAPHIC);
  EXPECT_EQ(base::string16(), formatted_omit_scheme)
      << "Explicitly test the 0-argument GURL constructor";
}

TEST(TextEliderTest, FormatOriginForSecurityDisplay) {
  for (size_t i = 0; i < arraysize(common_tests); ++i) {
    base::string16 formatted = url_formatter::FormatOriginForSecurityDisplay(
        url::Origin(GURL(common_tests[i].input)));
    EXPECT_EQ(base::WideToUTF16(common_tests[i].output), formatted)
        << common_tests[i].description;

    base::string16 formatted_omit_web_scheme =
        url_formatter::FormatOriginForSecurityDisplay(
            url::Origin(GURL(common_tests[i].input)),
            url_formatter::SchemeDisplay::OMIT_HTTP_AND_HTTPS);
    EXPECT_EQ(base::WideToUTF16(common_tests[i].output_omit_web_scheme),
              formatted_omit_web_scheme)
        << common_tests[i].description;

    base::string16 formatted_omit_cryptographic_scheme =
        url_formatter::FormatOriginForSecurityDisplay(
            url::Origin(GURL(common_tests[i].input)),
            url_formatter::SchemeDisplay::OMIT_CRYPTOGRAPHIC);
    EXPECT_EQ(
        base::WideToUTF16(common_tests[i].output_omit_cryptographic_scheme),
        formatted_omit_cryptographic_scheme)
        << common_tests[i].description;
  }

  const OriginTestData tests[] = {
      {"File URI", "file:///usr/example/file.html", L"file://", L"file://",
       L"file://"},
      {"File URI with hostname", "file://localhost/usr/example/file.html",
       L"file://localhost", L"file://localhost", L"file://localhost"},
      {"UNC File URI 1", "file:///CONTOSO/accounting/money.xls", L"file://",
       L"file://", L"file://"},
      {"UNC File URI 2",
       "file:///C:/Program%20Files/Music/Web%20Sys/main.html?REQUEST=RADIO",
       L"file://", L"file://", L"file://"},
      {"Invalid IPv6 address", "https://[2001:db8:0:1]/", L"", L"", L""},
      {"HTTP filesystem: URL with path",
       "filesystem:http://www.google.com/temporary/test.html",
       L"http://www.google.com", L"www.google.com", L"http://www.google.com"},
      {"File filesystem: URL with path",
       "filesystem:file://localhost/temporary/stuff/test.html?z=fun&goat=billy",
       L"file://", L"file://", L"file://"},
      {"Invalid scheme 1", "twelve://www.cyber.org/wow.php", L"", L"", L""},
      {"Invalid scheme 2", "://www.cyber.org/wow.php", L"", L"", L""},
      {"Invalid port 1", "https://173.194.65.103:000", L"", L"", L""},
      {"Invalid port 2", "https://173.194.65.103:gruffle", L"", L"", L""},
      {"Blob URL",
       "blob:http://www.html5rocks.com/4d4ff040-6d61-4446-86d3-13ca07ec9ab9",
       L"http://www.html5rocks.com", L"www.html5rocks.com",
       L"http://www.html5rocks.com"}};

  for (size_t i = 0; i < arraysize(tests); ++i) {
    base::string16 formatted = url_formatter::FormatOriginForSecurityDisplay(
        url::Origin(GURL(tests[i].input)));
    EXPECT_EQ(base::WideToUTF16(tests[i].output), formatted)
        << tests[i].description;

    base::string16 formatted_omit_web_scheme =
        url_formatter::FormatOriginForSecurityDisplay(
            url::Origin(GURL(tests[i].input)),
            url_formatter::SchemeDisplay::OMIT_HTTP_AND_HTTPS);
    EXPECT_EQ(base::WideToUTF16(tests[i].output_omit_web_scheme),
              formatted_omit_web_scheme)
        << tests[i].description;

    base::string16 formatted_omit_cryptographic_scheme =
        url_formatter::FormatOriginForSecurityDisplay(
            url::Origin(GURL(tests[i].input)),
            url_formatter::SchemeDisplay::OMIT_CRYPTOGRAPHIC);
    EXPECT_EQ(base::WideToUTF16(tests[i].output_omit_cryptographic_scheme),
              formatted_omit_cryptographic_scheme)
        << tests[i].description;
  }

  base::string16 formatted =
      url_formatter::FormatOriginForSecurityDisplay(url::Origin(GURL()));
  EXPECT_EQ(base::string16(), formatted)
      << "Explicitly test the url::Origin which takes an empty, invalid URL";

  base::string16 formatted_omit_scheme =
      url_formatter::FormatOriginForSecurityDisplay(
          url::Origin(GURL()),
          url_formatter::SchemeDisplay::OMIT_HTTP_AND_HTTPS);
  EXPECT_EQ(base::string16(), formatted_omit_scheme)
      << "Explicitly test the url::Origin which takes an empty, invalid URL";

  formatted_omit_scheme = url_formatter::FormatOriginForSecurityDisplay(
      url::Origin(GURL()), url_formatter::SchemeDisplay::OMIT_CRYPTOGRAPHIC);
  EXPECT_EQ(base::string16(), formatted_omit_scheme)
      << "Explicitly test the url::Origin which takes an empty, invalid URL";
}

// Verify that a URL passes through an explicit sequence of truncated strings as
// it is progressively shortened. This helps guarantee that transitional corner
// cases are handled properly. The original URL is the first string in the
// vector.
void RunProgressiveElisionTest(
    const std::vector<std::vector<std::string>>& testcases,
    bool preserve_filename) {
  const gfx::FontList font_list;

  for (const auto& strings : testcases) {
    ASSERT_GT(strings.size(), 0u);

    // The first string in the testcase is the original URL.
    const GURL url(strings[0]);
    const size_t total_width =
        gfx::GetStringWidth(base::UTF8ToUTF16(strings[0]), font_list);

    // Build a vector of all unique strings encountered as the URL is elided to
    // progressively shorter field widths, one pixel at a time.
    std::vector<base::string16> results;
    for (size_t width = total_width; width > 0; --width) {
      base::string16 elided;
      if (preserve_filename) {
        elided = url_formatter::ElideUrl(url, font_list, width);
      } else {
        url::Parsed parsed;
        elided = url_formatter::SimpleElideUrl(url, font_list, width, &parsed);
      }
      if (results.empty() || elided != results.back())
        results.push_back(elided);
    }
    EXPECT_GE(results.size(), strings.size());
    for (size_t i = 0; i < strings.size(); ++i) {
      EXPECT_EQ(base::UTF8ToUTF16(strings[i]), results[i]);
    }
  }
}

// Test eliding of commonplace URLs.
TEST(TextEliderTest, TestSimpleProgressiveEliding) {
  const std::string kEllipsis(gfx::kEllipsis);

  const std::vector<std::vector<std::string>> testcases = {
      {
          {"https://www.abc.com/def/?query"},
          {"https://www.abc.com/def/?qu" + kEllipsis},
          {"https://www.abc.com/def/?q" + kEllipsis},
          {"https://www.abc.com/def/?" + kEllipsis},
          {"https://www.abc.com/def/" + kEllipsis},
          {"https://www.abc.com/def" + kEllipsis},
          {"https://www.abc.com/de" + kEllipsis},
          {"https://www.abc.com/d" + kEllipsis},
          {"https://www.abc.com/" + kEllipsis},
          {"www.abc.com/def/?q" + kEllipsis},
          {"www.abc.com/def/?" + kEllipsis},
          {"www.abc.com/def/" + kEllipsis},
          {"www.abc.com/def" + kEllipsis},
          {"www.abc.com/de" + kEllipsis},
          {"www.abc.com/d" + kEllipsis},
          {"www.abc.com/" + kEllipsis},
          {kEllipsis + "w.abc.com/" + kEllipsis},
          {kEllipsis + ".abc.com/" + kEllipsis},
          {kEllipsis + "abc.com/" + kEllipsis},
          {kEllipsis + "bc.com/" + kEllipsis},
          {kEllipsis + "c.com/" + kEllipsis},
          {kEllipsis + ".com/" + kEllipsis},
          {kEllipsis + "com/" + kEllipsis},
          {kEllipsis + "om/" + kEllipsis},
          {kEllipsis + "m/" + kEllipsis},
          {kEllipsis + "/" + kEllipsis},
          {kEllipsis},
          {""},
      },
  };
  RunProgressiveElisionTest(testcases, false);
}

// TODO(cjgrant): This test runs REALLY slowly. The concept is good, but we
// can't have a unit test take 5 seconds to run.

TEST(TextEliderTest, TestProgressiveEliding) {
  const std::string kEllipsis(gfx::kEllipsis);

  const std::vector<std::vector<std::string>> testcases = {
      {
          {"https://abc.com/dir1/dir2/file?query"},
          {"https://abc.com/dir1/dir2/file?qu" + kEllipsis},
          {"https://abc.com/dir1/dir2/file?q" + kEllipsis},
          {"https://abc.com/dir1/dir2/file?" + kEllipsis},
          {"https://abc.com/dir1/dir2/file" + kEllipsis},
          {"https://abc.com/dir1/" + kEllipsis + "/file?" + kEllipsis},
          {"https://abc.com/dir1/" + kEllipsis + "/file" + kEllipsis},
          {"https://abc.com/" + kEllipsis + "/file?qu" + kEllipsis},
          {"https://abc.com/" + kEllipsis + "/file?q" + kEllipsis},
          {"https://abc.com/" + kEllipsis + "/file?" + kEllipsis},
          {"https://abc.com/" + kEllipsis + "/file" + kEllipsis},
          {"https://abc.com/" + kEllipsis + "?qu" + kEllipsis},
          {"https://abc.com/" + kEllipsis + "?q" + kEllipsis},
          {"https://abc.com/" + kEllipsis + "?" + kEllipsis},
          {"https://abc.com/" + kEllipsis + "" + kEllipsis},
          {"https://abc.com/di" + kEllipsis},
          {"https://abc.com/d" + kEllipsis},
          {"https://abc.com/" + kEllipsis},
          {"abc.com/dir1/di" + kEllipsis},
          {"abc.com/dir1/d" + kEllipsis},
          {"abc.com/dir1/" + kEllipsis},
          {"abc.com/dir1" + kEllipsis},
          {"abc.com/dir" + kEllipsis},
          {"abc.com/di" + kEllipsis},
          {"abc.com/d" + kEllipsis},
          {"abc.com/" + kEllipsis},
          {kEllipsis + "c.com/" + kEllipsis},
          {kEllipsis + ".com/" + kEllipsis},
          {kEllipsis + "com/" + kEllipsis},
          {kEllipsis + "om/" + kEllipsis},
          {kEllipsis + "m/" + kEllipsis},
          {kEllipsis + "/" + kEllipsis},
          {kEllipsis},
          {""},
      },
      {
          {"https://abc.com/dir1/dir2/?query"},
          {"https://abc.com/dir1/dir2/?qu" + kEllipsis},
          {"https://abc.com/dir1/dir2/?q" + kEllipsis},
          {"https://abc.com/dir1/dir2/?" + kEllipsis},
          {"https://abc.com/dir1/dir2/" + kEllipsis},
          {"https://abc.com/" + kEllipsis + "/dir2/?" + kEllipsis},
          {"https://abc.com/" + kEllipsis + "/dir2/" + kEllipsis},
          {"https://abc.com/" + kEllipsis + "?query"},
      },
  };

  RunProgressiveElisionTest(testcases, true);
}

// Test eliding of commonplace URLs.
TEST(TextEliderTest, TestSimpleEliding) {
  const std::string kEllipsisStr(gfx::kEllipsis);
  std::vector<Testcase> testcases = {
      // HTTPS with path.
      {"https://www.google.com/intl/en/ads/",
       "https://www.google.com/intl/en/a" + kEllipsisStr},
      {"https://www.google.com/intl/en/ads/",
       "https://www.google.com/i" + kEllipsisStr},
      {"https://www.google.com/intl/en/ads/",
       "https://www.google.com/" + kEllipsisStr},
      {"https://www.google.com/intl/en/ads/",
       "www.google.com/intl" + kEllipsisStr},
      {"https://www.google.com/intl/en/ads/", "www.google.com/" + kEllipsisStr},
      {"https://www.google.com/intl/en/ads/",
       kEllipsisStr + "google.com/" + kEllipsisStr},
      {"https://www.google.com/intl/en/ads/",
       kEllipsisStr + "oogle.com/" + kEllipsisStr},
      {"https://www.google.com/intl/en/ads/",
       kEllipsisStr + "m/" + kEllipsisStr},
      {"https://www.google.com/intl/en/ads/",
       kEllipsisStr + "/" + kEllipsisStr},
      {"https://www.google.com/intl/en/ads/", kEllipsisStr},

      // HTTPS with no path.
      {"https://www.google.com/", "https://www.google.com"},
      {"https://www.google.com/", "www.google.com"},
      {"https://www.google.com/", kEllipsisStr + "google.com"},

      // HTTP (scheme is stripped first).
      {"http://www.google.com/intl/en/ads/", "www.google.com/intl/en/ads/"},
      {"http://www.google.com/intl/en/ads/",
       "www.google.com/intl" + kEllipsisStr},
      {"http://www.google.com/intl/en/ads/", "www.google.com/" + kEllipsisStr},
      {"http://www.google.com/intl/en/ads/",
       kEllipsisStr + "w.google.com/" + kEllipsisStr},

      // HTTP with no path.
      {"http://www.google.com/", "www.google.com"},
      {"http://www.google.com/", kEllipsisStr + "google.com"},

      // File URLs.
      {"file:///C:/path1/path2", "file:///C:/path1/path2"},
      {"file:///C:/path1/path2", "file:///C:/path1/" + kEllipsisStr},
      {"file:///C:/path1/path2", "fil" + kEllipsisStr},
  };

  RunElisionTest(testcases, false);
}

void RunElisionTestWithParsing(const std::vector<ParsedTestcase>& testcases) {
  const gfx::FontList font_list;
  for (const auto& testcase : testcases) {
    SCOPED_TRACE(testcase.input + " to " + testcase.output);

    const GURL url(testcase.input);
    const float available_width =
        gfx::GetStringWidthF(base::UTF8ToUTF16(testcase.output), font_list);

    url::Parsed parsed;
    auto elided =
        url_formatter::SimpleElideUrl(url, font_list, available_width, &parsed);

    EXPECT_EQ(base::UTF8ToUTF16(testcase.output), elided);

    // Verify Parsing.
    url::Parsed expected_parsed;
    for (const auto& component : testcase.components) {
      switch (component.type) {
        case url::Parsed::ComponentType::SCHEME:
          expected_parsed.scheme.begin = component.begin;
          expected_parsed.scheme.len = component.len;
          break;
        case url::Parsed::ComponentType::HOST:
          expected_parsed.host.begin = component.begin;
          expected_parsed.host.len = component.len;
          break;
        case url::Parsed::ComponentType::PATH:
          expected_parsed.path.begin = component.begin;
          expected_parsed.path.len = component.len;
          break;
        default:
          ASSERT_TRUE(false) << "Unhandled component";
      }
    }
    {
      // Explicitly check that all parts match.
      // TODO(cjgrant): Look through these using an array of descriptors.
      const url::Parsed& expected = expected_parsed;
      EXPECT_EQ(expected.scheme.begin, parsed.scheme.begin);
      EXPECT_EQ(expected.scheme.len, parsed.scheme.len);
      EXPECT_EQ(expected.username.begin, parsed.username.begin);
      EXPECT_EQ(expected.username.len, parsed.username.len);
      EXPECT_EQ(expected.password.begin, parsed.password.begin);
      EXPECT_EQ(expected.password.len, parsed.password.len);
      EXPECT_EQ(expected.host.begin, parsed.host.begin);
      EXPECT_EQ(expected.host.len, parsed.host.len);
      EXPECT_EQ(expected.port.begin, parsed.port.begin);
      EXPECT_EQ(expected.port.len, parsed.port.len);
      EXPECT_EQ(expected.path.begin, parsed.path.begin);
      EXPECT_EQ(expected.path.len, parsed.path.len);
      EXPECT_EQ(expected.query.begin, parsed.query.begin);
      EXPECT_EQ(expected.query.len, parsed.query.len);
      EXPECT_EQ(expected.ref.begin, parsed.ref.begin);
      EXPECT_EQ(expected.ref.len, parsed.ref.len);
    }
  }
}

// Verify that during elision, the parsed URL components are properly modified.
TEST(SecureElisionTest, TestParsingAdjustments) {
  const std::string kEllipsisStr(gfx::kEllipsis);
  const std::vector<ParsedTestcase> testcases = {
      {"https://www.google.com/intl/en/ads/",
       "https://www.google.com/intl/en/ads/",
       {{url::Parsed::ComponentType::SCHEME, 0, 5},
        {url::Parsed::ComponentType::HOST, 8, 14},
        {url::Parsed::ComponentType::PATH, 22, 13}}},
      {"https://www.google.com/intl/en/ads/",
       "https://www.google.com/intl/en/a" + kEllipsisStr,
       {{url::Parsed::ComponentType::SCHEME, 0, 5},
        {url::Parsed::ComponentType::HOST, 8, 14},
        {url::Parsed::ComponentType::PATH, 22, 11}}},
      {"https://www.google.com/intl/en/ads/",
       "https://www.google.com/" + kEllipsisStr,
       {{url::Parsed::ComponentType::SCHEME, 0, 5},
        {url::Parsed::ComponentType::HOST, 8, 14},
        {url::Parsed::ComponentType::PATH, 22, 2}}},
      {"https://www.google.com/intl/en/ads/",
       kEllipsisStr + "google.com/" + kEllipsisStr,
       {{url::Parsed::ComponentType::HOST, 0, 11},
        {url::Parsed::ComponentType::PATH, 11, 2}}},
      {"https://www.google.com/intl/en/ads/",
       kEllipsisStr,
       {{url::Parsed::ComponentType::PATH, 0, 1}}},
      // HTTPS with no path.
      {"https://www.google.com/",
       "www.google.com",
       {{url::Parsed::ComponentType::HOST, 0, 14}}},
      {"https://www.google.com/",
       kEllipsisStr,
       {{url::Parsed::ComponentType::HOST, 0, 1}}},
      // HTTP with no path.
      {"http://www.google.com/",
       "www.google.com",
       {{url::Parsed::ComponentType::HOST, 0, 14}}},
      // File URLs.
      // TODO(cjgrant): Why does the path start at 7?
      {"file:///C:/path1/path2",
       "file:///C:/path1/" + kEllipsisStr,
       {{url::Parsed::ComponentType::SCHEME, 0, 4},
        {url::Parsed::ComponentType::PATH, 7, 11}}},
      {"file:///C:/path1/path2",
       "fi" + kEllipsisStr,
       {{url::Parsed::ComponentType::SCHEME, 0, 3}}},
  };

  RunElisionTestWithParsing(testcases);
}

}  // namespace
