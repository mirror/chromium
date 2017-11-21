// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_VR_ASSETS_H_
#define CHROME_BROWSER_VR_VR_ASSETS_H_

#include <stdint.h>
#include <memory>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"

namespace base {
class DictionaryValue;
class SequencedTaskRunner;
class SingleThreadTaskRunner;
class Version;
}  // namespace base

namespace vr {

constexpr uint32_t kCompatibleMajorVrAssetsComponentVersion = 0;

struct VrAssetsSingletonTrait;

class VrAssets {
 public:
  typedef base::OnceCallback<void(bool success, std::string environment)>
      OnAssetsLoadedCallback;

  static VrAssets* GetInstance();  // thread-safe.

  ~VrAssets();

  void OnComponentReady(
      const base::Version& version,
      const base::FilePath& install_dir,
      std::unique_ptr<base::DictionaryValue> manifest);  // thread-safe.

  // |on_loaded| will be called on the caller's thread.
  void LoadAssetsWhenComponentReady(
      OnAssetsLoadedCallback on_loaded);  // thread-safe.

 private:
  static void LoadAssetsTask(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      base::FilePath component_install_dir,
      OnAssetsLoadedCallback on_loaded);

  VrAssets();
  void OnComponentReadyInternal(base::FilePath install_dir);
  void LoadAssetsWhenComponentReadyInternal(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      OnAssetsLoadedCallback on_loaded);

  bool component_ready_ = false;
  base::FilePath component_install_dir_;
  OnAssetsLoadedCallback on_assets_loaded_;
  scoped_refptr<base::SequencedTaskRunner> on_assets_loaded_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;

  base::WeakPtrFactory<VrAssets> weak_ptr_factory_;

  friend struct VrAssetsSingletonTrait;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_VR_ASSETS_H_
