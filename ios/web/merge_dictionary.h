// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_MERGE_DICTIONARY_H_
#define IOS_WEB_MERGE_DICTIONARY_H_

#include "base/values.h"

namespace web {

// Similar to base::DictionaryValue::MergeDictionary(), except concatenates
// ListValue contents.
// This is explicitly not part of base::DictionaryValue at brettw's request.
void MergeDictionary(base::DictionaryValue* target,
                     const base::DictionaryValue* source);

}  // namespace web

#endif  // IOS_WEB_MERGE_DICTIONARY_H_
