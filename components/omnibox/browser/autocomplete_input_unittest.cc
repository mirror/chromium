// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/autocomplete_input.h"

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/metrics/proto/omnibox_event.pb.h"
#include "components/metrics/proto/omnibox_input_type.pb.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/third_party/mozilla/url_parse.h"

using base::ASCIIToUTF16;
using metrics::OmniboxEventProto;

TEST(AutocompleteInputTest, InputType) {
  struct test_data {
    const base::string16 input;
    const metrics::omnibox::OmniboxInputType type;
  } input_cases[] = {
    {base::string16(), metrics::omnibox::INVALID},
    {ASCIIToUTF16("?"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("?foo"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("?foo bar"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("?http://foo.com/bar"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("foo"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("localhost"), metrics::omnibox::URL},
    {ASCIIToUTF16("foo._"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("foo.c"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("foo.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("-foo.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("foo-.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("foo_.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("foo.-com"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("foo/"), metrics::omnibox::URL},
    {ASCIIToUTF16("foo/bar"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("foo/bar%00"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("foo/bar/"), metrics::omnibox::URL},
    {ASCIIToUTF16("foo/bar baz\\"), metrics::omnibox::URL},
    {ASCIIToUTF16("foo.com/bar"), metrics::omnibox::URL},
    {ASCIIToUTF16("foo;bar"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("foo/bar baz"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("foo bar.com"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("foo bar"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("foo+bar"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("foo+bar.com"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("\"foo:bar\""), metrics::omnibox::QUERY},
    {ASCIIToUTF16("link:foo.com"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("foo:81"), metrics::omnibox::URL},
    {ASCIIToUTF16("localhost:8080"), metrics::omnibox::URL},
    {ASCIIToUTF16("www.foo.com:81"), metrics::omnibox::URL},
    {ASCIIToUTF16("foo.com:123456"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("foo.com:abc"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("1.2.3.4:abc"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("user@foo"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("user@foo.com"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("user@foo/z"), metrics::omnibox::URL},
    {ASCIIToUTF16("user@foo/z z"), metrics::omnibox::URL},
    {ASCIIToUTF16("user@foo.com/z"), metrics::omnibox::URL},
    {ASCIIToUTF16("user:pass@"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("user:pass@!foo.com"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("user:pass@foo"), metrics::omnibox::URL},
    {ASCIIToUTF16("user:pass@foo.c"), metrics::omnibox::URL},
    {ASCIIToUTF16("user:pass@foo.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("user:pass@foo.com:81"), metrics::omnibox::URL},
    {ASCIIToUTF16("user:pass@foo:81"), metrics::omnibox::URL},
    {ASCIIToUTF16(".1"), metrics::omnibox::QUERY},
    {ASCIIToUTF16(".1/3"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("1.2"), metrics::omnibox::QUERY},
    {ASCIIToUTF16(".1.2"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("1.2/"), metrics::omnibox::URL},
    {ASCIIToUTF16("1.2/45"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("6008/32768"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("12345678/"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("123456789/"), metrics::omnibox::URL},
    {ASCIIToUTF16("1.2:45"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("user@1.2:45"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("user@foo:45"), metrics::omnibox::URL},
    {ASCIIToUTF16("user:pass@1.2:45"), metrics::omnibox::URL},
    {ASCIIToUTF16("host?query"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("host#"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("host#ref"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("host# ref"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("host/#ref"), metrics::omnibox::URL},
    {ASCIIToUTF16("host/?#ref"), metrics::omnibox::URL},
    {ASCIIToUTF16("host/?#"), metrics::omnibox::URL},
    {ASCIIToUTF16("host.com#ref"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://host#ref"), metrics::omnibox::URL},
    {ASCIIToUTF16("host/path?query"), metrics::omnibox::URL},
    {ASCIIToUTF16("host/path#ref"), metrics::omnibox::URL},
    {ASCIIToUTF16("en.wikipedia.org/wiki/Jim Beam"), metrics::omnibox::URL},
    // In Chrome itself, mailto: will get handled by ShellExecute, but in
    // unittest mode, we don't have the data loaded in the external protocol
    // handler to know this.
    // { ASCIIToUTF16("mailto:abuse@foo.com"), metrics::omnibox::URL },
    {ASCIIToUTF16("view-source:http://www.foo.com/"), metrics::omnibox::URL},
    {ASCIIToUTF16("javascript:alert(\"Hi there\");"), metrics::omnibox::URL},
#if defined(OS_WIN)
    {ASCIIToUTF16("C:\\Program Files"), metrics::omnibox::URL},
    {ASCIIToUTF16("\\\\Server\\Folder\\File"), metrics::omnibox::URL},
#endif  // defined(OS_WIN)
    {ASCIIToUTF16("http:foo"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://foo"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://foo._"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("http://foo.c"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://foo.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://foo_bar.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://foo/bar%00"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("http://foo/bar baz"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://-foo.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://foo-.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://foo_.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://foo.-com"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("http://_foo_.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://foo.com:abc"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("http://foo.com:123456"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("http://1.2.3.4:abc"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("http:user@foo.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://user@foo.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://user:pass@foo"), metrics::omnibox::URL},
    {ASCIIToUTF16("http:user:pass@foo.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://user:pass@foo.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://1.2"), metrics::omnibox::URL},
    {ASCIIToUTF16("http:user@1.2"), metrics::omnibox::URL},
    {ASCIIToUTF16("http://1.2/45"), metrics::omnibox::URL},
    {ASCIIToUTF16("http:ps/2 games"), metrics::omnibox::URL},
    {ASCIIToUTF16("https://foo.com"), metrics::omnibox::URL},
    {ASCIIToUTF16("127.0.0.1"), metrics::omnibox::URL},
    {ASCIIToUTF16("127.0.1"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("127.0.1/"), metrics::omnibox::URL},
    {ASCIIToUTF16("0.0.0"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("0.0.0.0"), metrics::omnibox::URL},
    {ASCIIToUTF16("0.0.0.1"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("http://0.0.0.1/"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("browser.tabs.closeButtons"), metrics::omnibox::UNKNOWN},
    {base::WideToUTF16(L"\u6d4b\u8bd5"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16("[2001:]"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("[2001:dB8::1]"), metrics::omnibox::URL},
    {ASCIIToUTF16("192.168.0.256"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("[foo.com]"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("filesystem:http://a.com/t/bar"), metrics::omnibox::URL},
    {ASCIIToUTF16("filesystem:http://a.com/"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("filesystem:file://"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("filesystem:http"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("filesystem:"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("chrome-search://"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("chrome-devtools:"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("about://f;"), metrics::omnibox::QUERY},
    {ASCIIToUTF16("://w"), metrics::omnibox::UNKNOWN},
    {ASCIIToUTF16(":w"), metrics::omnibox::UNKNOWN},
    {base::WideToUTF16(L".\u062A"), metrics::omnibox::UNKNOWN},
  };

  for (size_t i = 0; i < arraysize(input_cases); ++i) {
    SCOPED_TRACE(input_cases[i].input);
    AutocompleteInput input(input_cases[i].input, base::string16::npos,
                            std::string(), GURL(), base::string16(),
                            OmniboxEventProto::INVALID_SPEC, true, false, true,
                            true, false, TestSchemeClassifier());
    EXPECT_EQ(input_cases[i].type, input.type());
  }
}

TEST(AutocompleteInputTest, InputTypeWithDesiredTLD) {
  struct test_data {
    const base::string16 input;
    const metrics::omnibox::OmniboxInputType type;
    const std::string spec;  // Unused if not a URL.
  } input_cases[] = {
      {ASCIIToUTF16("401k"), metrics::omnibox::URL,
       std::string("http://www.401k.com/")},
      {ASCIIToUTF16("56"), metrics::omnibox::URL,
       std::string("http://www.56.com/")},
      {ASCIIToUTF16("1.2"), metrics::omnibox::URL,
       std::string("http://www.1.2.com/")},
      {ASCIIToUTF16("1.2/3.4"), metrics::omnibox::URL,
       std::string("http://www.1.2.com/3.4")},
      {ASCIIToUTF16("192.168.0.1"), metrics::omnibox::URL,
       std::string("http://www.192.168.0.1.com/")},
      {ASCIIToUTF16("999999999999999"), metrics::omnibox::URL,
       std::string("http://www.999999999999999.com/")},
      {ASCIIToUTF16("x@y"), metrics::omnibox::URL,
       std::string("http://x@www.y.com/")},
      {ASCIIToUTF16("x@y.com"), metrics::omnibox::URL,
       std::string("http://x@y.com/")},
      {ASCIIToUTF16("y/z z"), metrics::omnibox::URL,
       std::string("http://www.y.com/z%20z")},
      {ASCIIToUTF16("abc.com"), metrics::omnibox::URL,
       std::string("http://abc.com/")},
      {ASCIIToUTF16("foo bar"), metrics::omnibox::QUERY, std::string()},
  };

  for (size_t i = 0; i < arraysize(input_cases); ++i) {
    SCOPED_TRACE(input_cases[i].input);
    AutocompleteInput input(input_cases[i].input, base::string16::npos, "com",
                            GURL(), base::string16(),
                            OmniboxEventProto::INVALID_SPEC, true, false, true,
                            true, false, TestSchemeClassifier());
    EXPECT_EQ(input_cases[i].type, input.type());
    if (input_cases[i].type == metrics::omnibox::URL)
      EXPECT_EQ(input_cases[i].spec, input.canonicalized_url().spec());
  }
}

// This tests for a regression where certain input in the omnibox caused us to
// crash. As long as the test completes without crashing, we're fine.
TEST(AutocompleteInputTest, InputCrash) {
  AutocompleteInput input(base::WideToUTF16(L"\uff65@s"), base::string16::npos,
                          std::string(), GURL(), base::string16(),
                          OmniboxEventProto::INVALID_SPEC, true, false, true,
                          true, false, TestSchemeClassifier());
}

TEST(AutocompleteInputTest, ParseForEmphasizeComponent) {
  using url::Component;
  Component kInvalidComponent(0, -1);
  struct test_data {
    const base::string16 input;
    const Component scheme;
    const Component host;
  } input_cases[] = {
    { base::string16(), kInvalidComponent, kInvalidComponent },
    { ASCIIToUTF16("?"), kInvalidComponent, kInvalidComponent },
    { ASCIIToUTF16("?http://foo.com/bar"), kInvalidComponent,
        kInvalidComponent },
    { ASCIIToUTF16("foo/bar baz"), kInvalidComponent, Component(0, 3) },
    { ASCIIToUTF16("http://foo/bar baz"), Component(0, 4), Component(7, 3) },
    { ASCIIToUTF16("link:foo.com"), Component(0, 4), kInvalidComponent },
    { ASCIIToUTF16("www.foo.com:81"), kInvalidComponent, Component(0, 11) },
    { base::WideToUTF16(L"\u6d4b\u8bd5"), kInvalidComponent, Component(0, 2) },
    { ASCIIToUTF16("view-source:http://www.foo.com/"), Component(12, 4),
        Component(19, 11) },
    { ASCIIToUTF16("view-source:https://example.com/"),
      Component(12, 5), Component(20, 11) },
    { ASCIIToUTF16("view-source:www.foo.com"), kInvalidComponent,
        Component(12, 11) },
    { ASCIIToUTF16("view-source:"), Component(0, 11), kInvalidComponent },
    { ASCIIToUTF16("view-source:garbage"), kInvalidComponent,
        Component(12, 7) },
    { ASCIIToUTF16("view-source:http://http://foo"), Component(12, 4),
        Component(19, 4) },
    { ASCIIToUTF16("view-source:view-source:http://example.com/"),
        Component(12, 11), kInvalidComponent }
  };

  for (size_t i = 0; i < arraysize(input_cases); ++i) {
    SCOPED_TRACE(input_cases[i].input);
    Component scheme, host;
    AutocompleteInput::ParseForEmphasizeComponents(input_cases[i].input,
                                                   TestSchemeClassifier(),
                                                   &scheme,
                                                   &host);
    AutocompleteInput input(input_cases[i].input, base::string16::npos,
                            std::string(), GURL(), base::string16(),
                            OmniboxEventProto::INVALID_SPEC, true, false, true,
                            true, false, TestSchemeClassifier());
    EXPECT_EQ(input_cases[i].scheme.begin, scheme.begin);
    EXPECT_EQ(input_cases[i].scheme.len, scheme.len);
    EXPECT_EQ(input_cases[i].host.begin, host.begin);
    EXPECT_EQ(input_cases[i].host.len, host.len);
  }
}

TEST(AutocompleteInputTest, InputTypeWithCursorPosition) {
  struct test_data {
    const base::string16 input;
    size_t cursor_position;
    const base::string16 normalized_input;
    size_t normalized_cursor_position;
  } input_cases[] = {
    { ASCIIToUTF16("foo bar"), base::string16::npos,
      ASCIIToUTF16("foo bar"), base::string16::npos },

    // Regular case, no changes.
    { ASCIIToUTF16("foo bar"), 3, ASCIIToUTF16("foo bar"), 3 },

    // Extra leading space.
    { ASCIIToUTF16("  foo bar"), 3, ASCIIToUTF16("foo bar"), 1 },
    { ASCIIToUTF16("      foo bar"), 3, ASCIIToUTF16("foo bar"), 0 },
    { ASCIIToUTF16("      foo bar   "), 2, ASCIIToUTF16("foo bar   "), 0 },

    // A leading '?' used to be a magic character indicating the following
    // input should be treated as a "forced query", but now if such a string
    // reaches the AutocompleteInput parser the '?' should just be treated like
    // a normal character.
    { ASCIIToUTF16("?foo bar"), 2, ASCIIToUTF16("?foo bar"), 2 },
    { ASCIIToUTF16("  ?foo bar"), 4, ASCIIToUTF16("?foo bar"), 2 },
    { ASCIIToUTF16("?  foo bar"), 4, ASCIIToUTF16("?  foo bar"), 4 },
    { ASCIIToUTF16("  ?  foo bar"), 6, ASCIIToUTF16("?  foo bar"), 4 },
  };

  for (size_t i = 0; i < arraysize(input_cases); ++i) {
    SCOPED_TRACE(input_cases[i].input);
    AutocompleteInput input(
        input_cases[i].input, input_cases[i].cursor_position, std::string(),
        GURL(), base::string16(), OmniboxEventProto::INVALID_SPEC, true, false,
        true, true, false, TestSchemeClassifier());
    EXPECT_EQ(input_cases[i].normalized_input, input.text());
    EXPECT_EQ(input_cases[i].normalized_cursor_position,
              input.cursor_position());
  }
}
