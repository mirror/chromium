// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_CHROME_WIN_UTIL_WIN_SHELL_UTIL_IMPL_H_
#define CHROME_SERVICES_CHROME_WIN_UTIL_WIN_SHELL_UTIL_IMPL_H_

#include "base/macros.h"
#include "chrome/services/chrome_win_util/public/interfaces/win_shell_util.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace chrome {

class WinShellUtilImpl : public chrome::mojom::WinShellUtil {
 public:
  explicit WinShellUtilImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~WinShellUtilImpl() override;

 private:
  // chrome::mojom::WinShellUtil:
  void IsPinnedToTaskbar(IsPinnedToTaskbarCallback callback) override;

  void CallGetOpenFileName(
      uint32_t owner,
      uint32_t flags,
      const std::vector<std::tuple<base::string16, base::string16>>& filters,
      const base::FilePath& initial_directory,
      const base::FilePath& initial_filename,
      CallGetOpenFileNameCallback callback) override;

  void CallGetSaveFileName(
      uint32_t owner,
      uint32_t flags,
      const std::vector<std::tuple<base::string16, base::string16>>& filters,
      uint32_t one_based_filter_index,
      const base::FilePath& initial_directory,
      const base::FilePath& suggested_filename,
      const base::string16& default_extension,
      CallGetSaveFileNameCallback callback) override;

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(WinShellUtilImpl);
};

}  // namespace chrome

#endif  // CHROME_SERVICES_CHROME_WIN_UTIL_WIN_SHELL_UTIL_IMPL_H_
