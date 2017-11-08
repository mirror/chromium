// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef IOS_WEB_PUBLIC_WEB_STATE_FORM_ACTIVITY_PARAMS_H_
#define IOS_WEB_PUBLIC_WEB_STATE_FORM_ACTIVITY_PARAMS_H_

#include <string>

namespace web {

// Wraps information about event happening in a form.
struct FormActivityParams {
 public:
  FormActivityParams();
  FormActivityParams(const FormActivityParams& other);
  ~FormActivityParams();

  std::string form_name;
  std::string field_name;
  std::string field_type;
  std::string type;
  std::string value;
  bool input_missing = false;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_WEB_STATE_FORM_ACTIVITY_PARAMS_H_
