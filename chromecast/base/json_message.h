// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BASE_JSON_MESSAGE_H_
#define CHROMECAST_BASE_JSON_MESSAGE_H_

#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/values.h"

namespace chromecast {

class JsonMessage {
 public:
  JsonMessage();
  explicit JsonMessage(const std::string& json);
  explicit JsonMessage(scoped_ptr<base::DictionaryValue> dictionary);
  virtual ~JsonMessage();

  bool ToJson(std::string* json) const;
  bool Valid() const { return valid_; }

  // Transfers the ownership of the underlying dictionary.
  base::Value* Release();

  // Get the underlying value.
  const base::Value* Get() const;

 protected:
  scoped_ptr<base::DictionaryValue> dictionary_;
  // Helper functions to return a sub dictionary or list in the json message.
  // Will create it if it doesn't exist.
  void GetSubDictionary(const std::string& key,
                        base::DictionaryValue** out_value);
  void GetSubList(const std::string& key,
                  base::ListValue** out_value);

 private:
  bool valid_;

  DISALLOW_COPY_AND_ASSIGN(JsonMessage);
};

}  // namespace chromecast

#endif  // CHROMECAST_BASE_JSON_MESSAGE_H_
