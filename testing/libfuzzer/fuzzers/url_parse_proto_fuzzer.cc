// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// includes copied from url_parse_fuzzer.cc
#include "base/at_exit.h"
#include "base/i18n/icu_util.h"
#include "url/gurl.h"

// includes *not* copied from url_parse_fuzzer.cc
// Contains DEFINE_BINARY_PROTO_FUZZER, a macro we use to define our target
// function.
#include "third_party/libprotobuf-mutator/src/src/libfuzzer/libfuzzer_macro.h"
// Header information about the Protocol Buffer Url class.
#include "testing/libfuzzer/fuzzers/url.pb.h"

// The code using TestCase is copied from url_parse_fuzzer.cc
struct TestCase {
  TestCase() {
    CHECK(base::i18n::InitializeICU());
  }
  // used by ICU integration.
  base::AtExitManager at_exit_manager;
};

TestCase* test_case = new TestCase();

// Boilerplate code to silence libprotobuf-mutators logging.
protobuf_mutator::protobuf::LogSilencer log_silincer;

// Converts a URL in Protocol Buffer format to a url in string format.
// Since protobuf is a relatively simple format, fuzzing targets that do not
// accept protobufs (such as this one) will require code to convert from
// protobuf to the accepted format (string in this case).
std::string protobuf_to_string(const protobuf_mutator::Url& url) {
  // Build url_string piece by piece from url and then return it.
  std::string url_string = std::string("");

  if (url.has_scheme()) {  // Get scheme if we have one.
    // Append the scheme_name to the url. This may be empty.
    url_string += url.scheme().scheme_name();
    // Append a colon to the url. Colons are mandatory in schemes.
    url_string += ":";
    // If slash_1() was set to true by libFuzzer, append a "/".
    if (url.scheme().slash_1())
      url_string += "/";
    // If slash_2() was set to true by libFuzzer, append a "/".
    if (url.scheme().slash_2())
      url_string += "/";
  }

  // Get host. This is simple since hosts are simply strings according to our
  // definition.
  url_string += url.host();

  // As explained in url.proto: Ports must be started by a ":". If libFuzzer
  // included a query_string in a url, ensure that it is preceded by a ":".
  if (url.has_port_number()) {
    url_string += ":";
    url_string += url.port_number();
  }

  // Paths must be started by a "/". If libFuzzer included a path in the url,
  // ensure that it is preceded by a "/".
  if (url.has_path()) {
    url_string += "/";
    url_string += url.path();
  }

  // Queries must be started by a "?". If libFuzzer included a query_string in
  // the url, ensure that it is preceded by a "?".
  if (url.has_query_string()) {
    url_string += "?";
    url_string += url.query_string();
  }

  // Fragments must be started by a "#". If libFuzzer included a fragment_value
  // in the url ensure that it is preceded by a "#".
  if (url.has_fragment_value()) {
    url_string += "#";
    url_string += url.fragment_value();
  }

  return url_string;
}

// The target function. This is the equivalent of LLVMFuzzerTestOneInput in
// typical libFuzzer based fuzzers. It is passed our Url protobuf object that
// was mutated by libFuzzer, converts it to a string and then feeds it to url()
// for fuzzing.
DEFINE_BINARY_PROTO_FUZZER(const protobuf_mutator::Url& url_protobuf) {
  std::string url_string = protobuf_to_string(url_protobuf);
  GURL url(url_string);
}
