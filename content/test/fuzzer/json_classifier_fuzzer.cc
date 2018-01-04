// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Fuzzer for content/common/json_classifier.cc

#include <stddef.h>
#include <stdint.h>
#include <memory>

#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "content/common/json_classifier.h"

namespace content {

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
  base::CommandLine::Init(*argc, *argv);
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  base::StringPiece input(reinterpret_cast<const char*>(data), size);

  // Run the JsonClassifier on |data|.
  JsonClassifier input_classifier;
  input_classifier.Append(input);

  if (input_classifier.is_valid_json_so_far()) {
    std::string completion = input_classifier.GetCompletionSuffixForTesting();
    std::string completed_input = input.as_string();
    completed_input.append(input_classifier.GetCompletionSuffixForTesting());

    // |completed_input| ought to be valid JSON.
    JsonClassifier completed_input_classifier;
    completed_input_classifier.Append(completed_input);

    CHECK(completed_input_classifier.is_complete_json_value())
        << completed_input;
    CHECK(completed_input_classifier.is_valid_json_so_far());
    CHECK_EQ("", completed_input_classifier.GetCompletionSuffixForTesting());
    input_classifier.Append(completion);

    CHECK(input_classifier.is_complete_json_value());
    CHECK(input_classifier.is_valid_json_so_far());
    CHECK_EQ("", input_classifier.GetCompletionSuffixForTesting());

    // Validate |completed_input| against the JSON parser in base/.
    base::JSONReader json_reader(base::JSON_REPLACE_INVALID_CHARACTERS);
    auto json_reader_result = json_reader.ReadToValue(completed_input);
    CHECK_EQ(base::JSONReader::JSON_NO_ERROR, json_reader.error_code())
        << json_reader.GetErrorMessage() << ":" << completed_input;
    CHECK(json_reader_result) << json_reader.GetErrorMessage();

    // Write |json_reader_result| back to a string.
    std::string canonicalized_input;
    CHECK(base::JSONWriter::Write(*json_reader_result, &canonicalized_input));

    JsonClassifier canonicalized_input_classifier;
    canonicalized_input_classifier.Append(canonicalized_input);
    CHECK(canonicalized_input_classifier.is_complete_json_value());
  } else {
#if 0
    // Validate that this input is also rejected by the JSON parser in base/.
    base::JSONReader json_reader;
    auto json_reader_result = json_reader.ReadToValue(input);
    CHECK(!json_reader_result) << " " << input << " -> " << *json_reader_result;
    CHECK_NE(base::JSONReader::JSON_NO_ERROR, json_reader.error_code())
        << input;
#endif
  }

  return 0;
}

}  // namespace content
