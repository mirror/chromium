// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/assets.h"

#include "base/files/file_util.h"
#include "base/memory/singleton.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"

namespace vr {

namespace {

static const base::FilePath::CharType kEnvironmentFileName[] =
    FILE_PATH_LITERAL("environment");

}  // namespace

struct AssetsSingletonTrait : public base::DefaultSingletonTraits<Assets> {
  static Assets* New() { return new Assets(); }
  static void Delete(Assets* assets) { delete assets; }
};

AssetsUmaHelper::AssetsUmaHelper(scoped_refptr<base::SingleThreadTaskRunner> task_runner) : task_runner_(task_runner), weak_ptr_factory_(this) {

}

AssetsUmaHelper::~AssetsUmaHelper() = default;

void AssetsUmaHelper::OnEnterVrBrowsingMode() {
  task_runner_->PostTask(FROM_HERE, base::BindOnce(&AssetsUmaHelper::OnEnterVrBrowsingModeInternal, weak_ptr_factory_.GetWeakPtr()));
}

void AssetsUmaHelper::OnComponentReady() {
  LOG(INFO) << "AssetsUmaHelper::OnComponentReady " << entered_vr_browsing_mode_ << " " << (base::Time::Now() - enter_vr_browsing_mode_time_);
  component_ready_ = true;
  // TODO: UMA log time waited of if waited
}

void AssetsUmaHelper::OnEnterVrBrowsingModeInternal() {
  entered_vr_browsing_mode_ = true;
  enter_vr_browsing_mode_time_ = base::Time::Now();
  LOG(INFO) << "AssetsUmaHelper::OnEnterVrBrowsingMode";

  // TODO: UMA log if component is ready
}

// static
Assets* Assets::GetInstance() {
  return base::Singleton<Assets, AssetsSingletonTrait>::get();
}

void Assets::OnComponentReady(const base::Version& version,
                              const base::FilePath& install_dir,
                              std::unique_ptr<base::DictionaryValue> manifest) {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&Assets::OnComponentReadyInternal,
                                weak_ptr_factory_.GetWeakPtr(), install_dir));
}

void Assets::LoadWhenComponentReady(OnAssetsLoadedCallback on_loaded) {
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&Assets::LoadWhenComponentReadyInternal,
                                weak_ptr_factory_.GetWeakPtr(),
                                base::ThreadTaskRunnerHandle::Get(),
                                std::move(on_loaded)));
}

AssetsUmaHelper* Assets::GetUmaHelper() {
  return &uma_helper_;
}

// static
void Assets::LoadAssetsTask(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    const base::FilePath& component_install_dir,
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

Assets::Assets()
    : main_thread_task_runner_(content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::UI)), uma_helper_(main_thread_task_runner_), 
      weak_ptr_factory_(this) {
  DCHECK(main_thread_task_runner_.get());
}

Assets::~Assets() = default;

void Assets::OnComponentReadyInternal(const base::FilePath& install_dir) {
  component_install_dir_ = install_dir;
  component_ready_ = true;
  uma_helper_.OnComponentReady();
  if (on_assets_loaded_) {
    LoadWhenComponentReadyInternal(on_assets_loaded_task_runner_,
                                   std::move(on_assets_loaded_));
    on_assets_loaded_task_runner_ = nullptr;
  }
}

void Assets::LoadWhenComponentReadyInternal(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    OnAssetsLoadedCallback on_loaded) {
  if (component_ready_) {
    base::PostTaskWithTraits(
        FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
        base::BindOnce(&Assets::LoadAssetsTask, task_runner,
                       component_install_dir_, std::move(on_loaded)));
  } else {
    on_assets_loaded_ = std::move(on_loaded);
    on_assets_loaded_task_runner_ = task_runner;
  }
}

}  // namespace vr
