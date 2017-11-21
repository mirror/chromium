// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/vr_assets.h"

#include "base/files/file_util.h"
#include "base/memory/singleton.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"

namespace vr {

namespace {

static const base::FilePath::CharType kEnvironmentFileName[] =
    FILE_PATH_LITERAL("environment");

}  // namespace

struct VrAssetsSingletonTrait : public base::DefaultSingletonTraits<VrAssets> {
  static VrAssets* New() { return new VrAssets(); }
};

// static
VrAssets* VrAssets::GetInstance() {
  return base::Singleton<VrAssets, VrAssetsSingletonTrait>::get();
}

VrAssets::~VrAssets() = default;

void VrAssets::OnComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrAssets::OnComponentReadyInternal,
                                weak_ptr_factory_.GetWeakPtr(), install_dir));
}

void VrAssets::LoadAssetsWhenComponentReady(OnAssetsLoadedCallback on_loaded) {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&VrAssets::LoadAssetsWhenComponentReadyInternal,
                                weak_ptr_factory_.GetWeakPtr(),
                                base::SequencedTaskRunnerHandle::Get(),
                                std::move(on_loaded)));
}

// static
void VrAssets::LoadAssetsTask(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    base::FilePath component_install_dir,
    OnAssetsLoadedCallback on_loaded) {
  std::string environment;
  if (!base::ReadFileToString(
          component_install_dir.Append(kEnvironmentFileName), &environment)) {
    task_runner->PostTask(
        FROM_HERE, base::BindOnce(std::move(on_loaded), false, std::string()));
    return;
  }

  task_runner->PostTask(
      FROM_HERE, base::BindOnce(std::move(on_loaded), true, environment));
}

VrAssets::VrAssets()
    : main_thread_task_runner_(content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::UI)),
      weak_ptr_factory_(this) {
  DCHECK(main_thread_task_runner_.get());
}

void VrAssets::OnComponentReadyInternal(base::FilePath install_dir) {
  component_install_dir_ = install_dir;
  component_ready_ = true;
  if (on_assets_loaded_) {
    LoadAssetsWhenComponentReadyInternal(on_assets_loaded_task_runner_,
                                         std::move(on_assets_loaded_));
  }
}

void VrAssets::LoadAssetsWhenComponentReadyInternal(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    OnAssetsLoadedCallback on_loaded) {
  if (component_ready_) {
    base::PostTaskWithTraits(
        FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
        base::BindOnce(&VrAssets::LoadAssetsTask, task_runner,
                       component_install_dir_, std::move(on_loaded)));
  } else {
    on_assets_loaded_ = std::move(on_loaded);
    on_assets_loaded_task_runner_ = task_runner;
  }
}

}  // namespace vr