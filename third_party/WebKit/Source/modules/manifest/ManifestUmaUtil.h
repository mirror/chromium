// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_MODULES_MANIFEST_MANIFESTUMAUTIL_H_
#define THIRD_PARTY_WEBKIT_SOURCE_MODULES_MANIFEST_MANIFESTUMAUTIL_H_

namespace blink {
namespace mojom {
namespace blink {
class Manifest;
}
}  // namespace mojom

class ManifestUmaUtil {
 public:
  enum FetchFailureReason { FETCH_EMPTY_URL = 0, FETCH_UNSPECIFIED_REASON };

  // Record that the Manifest was successfully parsed. If it is an empty
  // Manifest, it will recorded as so and nothing will happen. Otherwise, the
  // presence of each properties will be recorded.
  static void ParseSucceeded(const mojom::blink::Manifest*);

  // Record that the Manifest parsing failed.
  static void ParseFailed();

  // Record that the Manifest fetching succeeded.
  static void FetchSucceeded();

  // Record that the Manifest fetching failed and takes the |reason| why it
  // failed.
  static void FetchFailed(FetchFailureReason);
};

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_MODULES_MANIFEST_MANIFESTUMAUTIL_H_
