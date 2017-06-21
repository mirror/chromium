// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/log/crash_resistant_net_log_observer.h"

#include <stdio.h>

#include <memory>
#include <set>
#include <utility>

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "net/log/net_log_entry.h"
#include "net/log/net_log_util.h"
#include "net/url_request/url_request_context.h"

namespace net {

CrashResistantNetLogObserver::CrashResistantNetLogObserver() {
}

CrashResistantNetLogObserver::~CrashResistantNetLogObserver() {
}

void CrashResistantNetLogObserver::StartObserving(
    NetLog* net_log,
    base::ScopedFILE file,
    base::Value* constants,
    URLRequestContext* url_request_context) {
  DCHECK(file.get());
  file_ = std::move(file);

  net_log->DeprecatedAddObserver(this, NetLogCaptureMode::Default());
}

void CrashResistantNetLogObserver::StopObserving(
    URLRequestContext* url_request_context) {
  net_log()->DeprecatedRemoveObserver(this);
}

void CrashResistantNetLogObserver::OnAddEntry(const NetLogEntry& entry) {
  std::unique_ptr<base::Value> value(entry.ParametersToValue());
  if (!value) { return; }

  base::DictionaryValue* dict;
  value->GetAsDictionary(&dict);

  std::string url;
  dict->GetString("url", &url);

  if (url.empty()) { return; }

  fprintf(file_.get(), "%s\n", url.c_str());
  fflush(file_.get());
}

}  // namespace net

