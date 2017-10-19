// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/core/common/indexed_ruleset.h"

#include <string>

#include "base/at_exit.h"
#include "base/i18n/icu_util.h"
#include "base/strings/string_piece.h"
#include "base/test/fuzzed_data_provider.h"
#include "components/subresource_filter/core/common/first_party_origin.h"
#include "components/url_pattern_index/url_pattern_index.h"
#include "url/gurl.h"
#include "url/origin.h"

struct TestCase {
  TestCase() { CHECK(base::i18n::InitializeICU()); }

  // used by ICU integration.
  base::AtExitManager at_exit_manager;
};

TestCase* test_case = new TestCase();

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  base::FuzzedDataProvider fuzzed_data(data, size);

  // Assume the first 32 bytes are a URL to check, but fuzz the size we consider
  // as the URL. This will make it easier for differring URL sizes to not affect
  // the subsequent indexed ruleset.
  uint32_t url_size = fuzzed_data.ConsumeUint32InRange(0, 32);
  bool disable_generic_rules = fuzzed_data.ConsumeBool();

  std::string url_piece = fuzzed_data.ConsumeBytes(32);
  GURL url_to_check = GURL(url_piece.substr(0, url_size));

  // Assume the rest of the data is a valid flatbuffer.
  std::string remaining_bytes = fuzzed_data.ConsumeRemainingBytes();
  const uint8_t* remaining_data =
      reinterpret_cast<const uint8_t*>(remaining_bytes.data());
  if (!subresource_filter::IndexedRulesetMatcher::Verify(
          remaining_data, remaining_bytes.size())) {
    return 0;
  }

  subresource_filter::IndexedRulesetMatcher matcher(remaining_data,
                                                    remaining_bytes.size());
  // TODO(csharrison): Consider fuzzing things like the parent origin, the
  // activation type, and the element type.
  matcher.ShouldDisableFilteringForDocument(
      url_to_check, url::Origin(),
      url_pattern_index::proto::ACTIVATION_TYPE_DOCUMENT);
  matcher.ShouldDisallowResourceLoad(
      url_to_check, subresource_filter::FirstPartyOrigin(url::Origin()),
      url_pattern_index::proto::ELEMENT_TYPE_SCRIPT, disable_generic_rules);
  return 0;
}
