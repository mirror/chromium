// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "base/at_exit.h"
#include "base/i18n/icu_util.h"
#include "third_party/libprotobuf-mutator/src/src/libfuzzer/libfuzzer_macro.h"
#include "url/gurl.h"
#include "testing/libfuzzer/fuzzers/url.pb.h"

struct TestCase {
  TestCase() {
    CHECK(base::i18n::InitializeICU());
  }

  // used by ICU integration.
  base::AtExitManager at_exit_manager;
};

TestCase* test_case = new TestCase();

protobuf_mutator::protobuf::LogSilencer log_silincer;

std::string convert_protobuf_to_string(const protobuf_mutator::Url& url) {
  std::string url_string = std::string("");

  // Get scheme.
  if (url.has_scheme()) {
    url_string += url.scheme().scheme_name();
    url_string += ":";
    if (url.scheme().slash_1())
       url_string += "/";
    if (url.scheme().slash_2())
      url_string += "/";
  }

  // Get credentials.
  if (url.has_credentials()) {
    url_string += url.credentials().username();
    url_string += ":";
    url_string += url.credentials().password();
    url_string += "@";
  }

  // Get host.
  url_string += url.host();

  // Get port.
  if (url.has_port_number()) {
    url_string += ":";
    url_string += url.port();
  }

  // Get path.
  if (url.has_path()) {
    url_string += "/";
    url_string += url.path().path();
  }

  // Get query.
  if (url.has_query_string()) {
    url_string += "?";
    url_string += url.query_string();
  }

  // Get fragment.
  if (url.has_fragment_value()) {
    url_string += "#";
    url_string += url.fragment_value();
  }

  return url_string;
}

DEFINE_BINARY_PROTO_FUZZER(const protobuf_mutator::Url& url_protobuf) {
  std::string url_string = protobuff_to_string(url_protobuf);
  GURL url(url_string);
}
