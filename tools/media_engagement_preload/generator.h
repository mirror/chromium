// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_MEDIA_ENGAGEMENT_PRELOAD_GENERATOR_H_
#define TOOLS_MEDIA_ENGAGEMENT_PRELOAD_GENERATOR_H_

#include <set>
#include <string>

#include "base/strings/string_piece.h"

namespace media {

// Parses the |json| string; populates a set of strings in |output|.
// The return value is whether the parsing succeeded.
bool ParseJSON(base::StringPiece json, std::set<std::string>* output);

// Stores a Trie of |entries| in a PreloadedData proto message.
std::string GenerateToProto(const std::set<std::string>& entries);

}  // namespace media

#endif  // TOOLS_MEDIA_ENGAGEMENT_PRELOAD_GENERATOR_H_
