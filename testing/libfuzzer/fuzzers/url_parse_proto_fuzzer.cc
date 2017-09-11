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

std::string convert_protobuf_to_url(const protobuf_mutator::Url& url) {
  std::string url_string = std::string("");

  // Get scheme.
  url_string += url.scheme().scheme();
  url_string += url.scheme().colon();
  url_string += url.scheme().slash_1();
  url_string += url.scheme().slash_2();

  // Get credentials.
  url_string += url.credentials().username();
  url_string += url.credentials().colon();
  url_string += url.credentials().password();
  url_string += url.credentials().at_sign();

  // Get host.
  url_string += url.host();

  // Get port.
  url_string += url.port().colon();
  url_string += url.port().port();

  // Get path.
  url_string += url.path().root();
  url_string += url.path().path();

  // Get guery.
  url_string += url.query().question_mark();
  url_string += url.query().query();

  // Get fragment.
  url_string += url.fragment().hash();
  url_string += url.fragment().fragment();

  return url_string;
}

DEFINE_BINARY_PROTO_FUZZER(const protobuf_mutator::Url& url_protobuf) {
  std::string url_string = convert_protobuf_to_url(url_protobuf);
  GURL url(url_string);
}
