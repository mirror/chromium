// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MODULE_DATABASE_CONFLICTS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_MODULE_DATABASE_CONFLICTS_HANDLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/conflicts/module_database_observer_win.h"
#include "content/public/browser/web_ui_message_handler.h"

class ModuleDatabase;

namespace base {
class Listvalue;
}

class ModuleDatabaseConflictsHandler : public content::WebUIMessageHandler,
                                       public ModuleDatabaseObserver {
 public:
  ModuleDatabaseConflictsHandler();
  ~ModuleDatabaseConflictsHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

  // Callback for the "requestModuleList" message.
  void HandleRequestModuleList(const base::ListValue* args);

 private:
  // ModuleDatabaseObserver:
  void OnNewModuleFound(const ModuleInfoKey& module_key,
                        const ModuleInfoData& module_data) override;
  void OnModuleDatabaseIdle() override;

  ScopedObserver<ModuleDatabase, ModuleDatabaseObserver> observer_;

  // The id of the callback that will get invoked with the module list.
  std::string module_list_callback_id_;

  // Contains the module list in JSON format.
  std::unique_ptr<base::ListValue> module_json_list_;

  DISALLOW_COPY_AND_ASSIGN(ModuleDatabaseConflictsHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_MODULE_DATABASE_CONFLICTS_HANDLER_H_
