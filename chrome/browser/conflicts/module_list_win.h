// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_MODULE_LIST_WIN_H_
#define CHROME_BROWSER_CONFLICTS_MODULE_LIST_WIN_H_

#include <memory>

#include "base/macros.h"

namespace chrome {
namespace conflicts {
class ModuleList;
}  // namespace conflicts
}  // namespace chrome

struct ModuleInfoKey;
struct ModuleInfoData;

// Encapsulates a proto message chrome::conflicts::ModuleList.
class ModuleList {
 public:
  explicit ModuleList(
      std::unique_ptr<chrome::conflicts::ModuleList> module_list);
  ~ModuleList();

  bool IsBlacklisted(const ModuleInfoKey& module_key,
                     const ModuleInfoData& module_data);
  bool IsWhitelisted(const ModuleInfoKey& module_key,
                     const ModuleInfoData& module_data);
  bool ShouldShowWarning(const ModuleInfoKey& module_key,
                         const ModuleInfoData& module_data);

 private:
  std::unique_ptr<chrome::conflicts::ModuleList> module_list_;

  DISALLOW_COPY_AND_ASSIGN(ModuleList);
};

#endif  // CHROME_BROWSER_CONFLICTS_MODULE_LIST_WIN_H_
