// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_TEST_REFERENCE_READER_H_
#define CHROME_INSTALLER_ZUCCHINI_TEST_REFERENCE_READER_H_

#include <stddef.h>

#include <vector>

#include "base/optional.h"
#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

// A trivial ReferenceReader that reads injected references.
class TestReferenceReader : public ReferenceReader {
 public:
  explicit TestReferenceReader(const std::vector<Reference>& refs)
      : references_(refs) {}
  ~TestReferenceReader() override = default;

  base::Optional<Reference> GetNext() override {
    if (index_ == references_.size())
      return base::nullopt;
    return references_[index_++];
  }

 private:
  std::vector<Reference> references_;
  size_t index_ = 0;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_TEST_REFERENCE_READER_H_
