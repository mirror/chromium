// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/reporting/reporting_cache.h"
#include "net/reporting/reporting_client.h"
#include "net/reporting/reporting_header_parser.h"
#include "net/reporting/reporting_test_util.h"
#include "net/reporting/reporting_policy.pb.h"
#include "url/gurl.h"

#include "testing/libfuzzer/proto/json_proto_converter.h"

// TODO(mmoroz): the next include violates checkdeps rules, need to resolve it.
#include "third_party/libprotobuf-mutator/src/src/libfuzzer/libfuzzer_macro.h"


// Silence logging from the protobuf library.
protobuf_mutator::protobuf::LogSilencer log_silencer;

const GURL kUrl_ = GURL("https://origin/path");

// TODO: randomize policy based on the data.
const net::ReportingPolicy kPolicy;
net::TestReportingContext kContext(kPolicy);

namespace net_reporting_header_parser_fuzzer {

int FuzzReportingHeaderParser(const std::string& data) {
  net::ReportingHeaderParser::ParseHeader(&kContext, kUrl_, data.c_str());
  std::vector<const net::ReportingClient*> clients;
  kContext.cache()->GetClients(&clients);
  if (clients.empty()) {
    return 0;
  }

  return 0;
}

//DEFINE_BINARY_PROTO_FUZZER(const json_proto::JsonObject& json_object) {
DEFINE_BINARY_PROTO_FUZZER(
    const net_reporting_policy_proto::ReportingHeaderParserFuzzInput& input) {
  json_proto::JsonProtoConverter converter;
  auto data = converter.Convert(input.headers());
  FuzzReportingHeaderParser(data);
}

} // namespace net_reporting_header_parser_fuzzer
