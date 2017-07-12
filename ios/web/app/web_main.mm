// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/app/web_main.h"
#include "ios/web/public/app/web_main_runner.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

WebMainParams::WebMainParams(WebMainDelegate* delegate)
    : delegate(delegate),
      register_exit_manager(true),
      get_task_scheduler_init_params_callback(nullptr),
      argc(0),
      argv(nullptr) {}

WebMainParams::~WebMainParams() = default;

WebMain::WebMain(std::unique_ptr<WebMainParams> params) {
  web_main_runner_.reset(WebMainRunner::Create());
  web_main_runner_->Initialize(std::move(params));
}

WebMain::~WebMain() {
  web_main_runner_->ShutDown();
}

}  // namespace web
