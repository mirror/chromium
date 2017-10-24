// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "tools/gn/input_file.h"
#include "tools/gn/parser.h"
#include "tools/gn/source_file.h"
#include "tools/gn/tokenizer.h"

namespace {

enum { kMaxContentDepth = 100 };

// Some auto generated input is too unreasonable for fuzzing GN.
// We see stack overflow when the parser hits a really deeply nested input.
void SanityCheckContent(std::string& content) {
  // Iterate through the test content string.  Sanity check the depth in the
  // test data.  If it gets unrealistically deep, bypass test.
  int depth = 0;
  for (auto&& character : content) {
    if (character == '{')
      ++depth;
    if (character == '}')
      --depth;

    if (depth >= kMaxContentDepth)
      return 0;
  }
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const unsigned char* data, size_t size) {
  std::string content(reinterpret_cast<const char*>(data), size);
  SanityCheckContent(content);

  SourceFile source;
  InputFile input(source);
  input.SetContents(content);

  Err err;
  std::vector<Token> tokens = Tokenizer::Tokenize(&input, &err);

  if (!err.has_error())
    Parser::Parse(tokens, &err);

  return 0;
}
