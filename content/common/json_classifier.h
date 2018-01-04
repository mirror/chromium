// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <string>

#include "base/containers/stack_container.h"
#include "base/macros.h"
#include "base/strings/string_piece.h"
#include "content/common/content_export.h"

namespace content {

class CONTENT_EXPORT JsonClassifier {
 public:
  enum State : uint8_t;

  // Static helper functions.
  // static base::Optional<base::Value::TYPE> IsValidJson(base::StringPiece
  // data); static CrossSiteDocumentClassifier::Result
  // JsonDict(base::StringPiece data);
  JsonClassifier();
  ~JsonClassifier();

  JsonClassifier* Append(base::StringPiece data);

  // Returns true if the stream of data fed to this object thus far is a valid,
  // complete JSON value. This value is only meaningful once the entire stream
  // has been fed to the classifier.
  bool is_complete_json_value() const;
  bool is_valid_json_so_far() const { return state_ != 0; }

  // Returns true if
  bool is_not_valid_javascript() const { return is_not_valid_javascript_; }

  // If is_valid_so_far() is true, this method returns a string that, when
  // appended to the input, ought to cause is_complete_json_value() to become
  // true.
  std::string GetCompletionSuffixForTesting() const;

 private:
  bool Push(State state);
  State Pop();

  JsonClassifier* Fail();

  // This is a base::StackVector so that when we have fewer than 16 levels of
  // nesting it uses inline storage.
  bool is_not_valid_javascript_;
  State state_;
  base::StackVector<State, 16> stack_;
};

}  // namespace content
