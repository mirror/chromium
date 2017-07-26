// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/renovations/page_renovator.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"

namespace offline_pages {

PageRenovator::PageRenovator(PageRenovationLoader* renovation_loader,
                             const GURL& request_url,
                             ScriptInjector* script_injector)
    : renovation_loader_(renovation_loader),
      request_url_(request_url),
      script_injector_(script_injector),
      script_(base::UTF8ToUTF16("console.log(\"No renovation script!\");")) {
  DCHECK(renovation_loader);

  PrepareScript();
}

PageRenovator::~PageRenovator() {}

void PageRenovator::RunRenovations(base::Closure callback) {
  // Prepare callback and inject combined script.
  base::RepeatingCallback<void(const base::Value*)> cb = base::Bind(
      [](base::Closure callback, const base::Value*) {
        if (callback)
          callback.Run();
      },
      std::move(callback));

  script_injector_->Inject(script_, cb);
}

void PageRenovator::PrepareScript() {
  std::vector<std::string> renovation_keys;

  // Pick which renovations to run.
  for (const std::unique_ptr<PageRenovation>& renovation_ptr :
       renovation_loader_->renovations()) {
    if (renovation_ptr->ShouldRun(request_url_)) {
      renovation_keys.push_back(renovation_ptr->GetID());
    }
  }

  // Store combined renovation script. TODO(collinbaker): handle
  // failed GetRenovationScript call.
  renovation_loader_->GetRenovationScript(renovation_keys, &script_);
}

}  // namespace offline_pages
