// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CONTENT_RENOVATIONS_SCRIPT_INJECTOR_H_
#define COMPONENTS_OFFLINE_PAGES_CONTENT_RENOVATIONS_SCRIPT_INJECTOR_H_

#include <memory>

#include "base/callback.h"
#include "base/strings/string16.h"
#include "content/public/browser/web_contents.h"

namespace offline_pages {

// The ScriptInjector handles executing renovation JavaScript in a
// page when the page is ready.
class ScriptInjector {
 public:
  // Creates a ScriptInjectorImpl instance. Takes the web_contents
  // into which scripts will be injected, and the isolated world id in
  // which the scripts will run. For chrome, the world_id should be
  // chrome::ISOLATED_WORLD_ID_CHROME_INTERNAL.
  static std::unique_ptr<ScriptInjector> Create(
      content::WebContents* web_contents,
      int world_id);

  // Adds a script to be injected into page when ready. When the
  // script has finished executing, the callback is called.
  virtual void AddScript(base::string16 script,
                         base::RepeatingClosure callback) = 0;

  virtual ~ScriptInjector() {}
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CONTENT_RENOVATIONS_SCRIPT_INJECTOR_H_
