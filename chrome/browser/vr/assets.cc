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
#include "base/version.h"
#include "chrome/browser/vr/metrics_helper.h"
#include "chrome/browser/vr/model/loaded_assets.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/codec/png_codec.h"

namespace vr {

namespace {

static const base::FilePath::CharType kBackgroundBaseFilename[] =
    FILE_PATH_LITERAL("background");
static const base::FilePath::CharType kNormalGradientBaseFilename[] =
    FILE_PATH_LITERAL("normal_gradient");
static const base::FilePath::CharType kIncognitoGradientBaseFilename[] =
    FILE_PATH_LITERAL("incognito_gradient");
static const base::FilePath::CharType kFullscreenGradientBaseFilename[] =
    FILE_PATH_LITERAL("fullscreen_gradient");
static const base::FilePath::CharType kPngExtension[] =
    FILE_PATH_LITERAL("png");
static const base::FilePath::CharType kJpegExtension[] =
    FILE_PATH_LITERAL("jpeg");

}  // namespace

struct AssetsSingletonTrait : public base::DefaultSingletonTraits<Assets> {
  static Assets* New() { return new Assets(); }
  static void Delete(Assets* assets) { delete assets; }
};

// static
Assets* Assets::GetInstance() {
  return base::Singleton<Assets, AssetsSingletonTrait>::get();
}

void Assets::OnComponentReady(const base::Version& version,
                              const base::FilePath& install_dir,
                              std::unique_ptr<base::DictionaryValue> manifest) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&Assets::OnComponentReadyInternal,
                     weak_ptr_factory_.GetWeakPtr(), version, install_dir));
}

void Assets::Load(OnAssetsLoadedCallback on_loaded) {
  main_thread_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&Assets::LoadInternal, weak_ptr_factory_.GetWeakPtr(),
                     base::ThreadTaskRunnerHandle::Get(),
                     std::move(on_loaded)));
}

MetricsHelper* Assets::GetMetricsHelper() {
  // If we instantiate metrics_helper_ in the constructor all functions of
  // MetricsHelper must be called in a valid sequence from the thread the
  // constructor ran on. However, the assets class can be instantiated from any
  // thread. To avoid the aforementioned restriction, create metrics_helper_ the
  // first time it is used and, thus, give the caller control over when the
  // sequence starts.
  if (!metrics_helper_) {
    metrics_helper_ = base::MakeUnique<MetricsHelper>();
  }
  return metrics_helper_.get();
}

bool Assets::ComponentReady() {
  DCHECK(main_thread_task_runner_->BelongsToCurrentThread());
  return component_ready_;
}

void Assets::SetOnComponentReadyCallback(
    const base::RepeatingCallback<void()>& on_component_ready) {
  DCHECK(main_thread_task_runner_->BelongsToCurrentThread());
  on_component_ready_callback_ = on_component_ready;
}

AssetsLoadStatus LoadImage(const base::FilePath& component_install_dir,
                           const base::FilePath::CharType* base_file,
                           std::unique_ptr<SkBitmap>* out_image) {
  bool is_png = false;
  std::string encoded_file_content;

  base::FilePath file_path = component_install_dir.Append(base_file);

  if (base::PathExists(file_path.AddExtension(kPngExtension))) {
    file_path = file_path.AddExtension(kPngExtension);
    is_png = true;
  } else if (base::PathExists(file_path.AddExtension(kJpegExtension))) {
    file_path = file_path.AddExtension(kJpegExtension);
  } else {
    return AssetsLoadStatus::kNotFound;
  }

  if (!base::ReadFileToString(file_path, &encoded_file_content)) {
    return AssetsLoadStatus::kParseFailure;
  }

  if (is_png) {
    (*out_image) = base::MakeUnique<SkBitmap>();
    if (!gfx::PNGCodec::Decode(
            reinterpret_cast<const unsigned char*>(encoded_file_content.data()),
            encoded_file_content.size(), out_image->get())) {
      (*out_image) = nullptr;
    }
  } else {
    (*out_image) = gfx::JPEGCodec::Decode(
        reinterpret_cast<const unsigned char*>(encoded_file_content.data()),
        encoded_file_content.size());
  }

  if (!out_image->get()) {
    return AssetsLoadStatus::kInvalidContent;
  }

  return AssetsLoadStatus::kSuccess;
}

// static
void Assets::LoadAssetsTask(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    const base::Version& component_version,
    const base::FilePath& component_install_dir,
    OnAssetsLoadedCallback on_loaded) {
  auto assets = base::MakeUnique<LoadedAssets>();
  AssetsLoadStatus status = AssetsLoadStatus::kSuccess;

  status = LoadImage(component_install_dir, kBackgroundBaseFilename,
                     &assets->background);

  if (status == AssetsLoadStatus::kSuccess) {
    LoadImage(component_install_dir, kNormalGradientBaseFilename,
              &assets->normal_gradient);
  }
  if (status == AssetsLoadStatus::kSuccess) {
    LoadImage(component_install_dir, kIncognitoGradientBaseFilename,
              &assets->incognito_gradient);
  }
  if (status == AssetsLoadStatus::kSuccess) {
    LoadImage(component_install_dir, kFullscreenGradientBaseFilename,
              &assets->fullscreen_gradient);
  }

  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(on_loaded), AssetsLoadStatus::kSuccess,
                     std::move(assets), component_version));
}

Assets::Assets()
    : main_thread_task_runner_(content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::UI)),
      weak_ptr_factory_(this) {
  DCHECK(main_thread_task_runner_.get());
}

Assets::~Assets() = default;

void Assets::OnComponentReadyInternal(const base::Version& version,
                                      const base::FilePath& install_dir) {
  DCHECK(main_thread_task_runner_->BelongsToCurrentThread());
  component_version_ = version;
  component_install_dir_ = install_dir;
  component_ready_ = true;
  if (on_component_ready_callback_) {
    on_component_ready_callback_.Run();
  }
  GetMetricsHelper()->OnComponentReady(version);
}

void Assets::LoadInternal(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    OnAssetsLoadedCallback on_loaded) {
  DCHECK(main_thread_task_runner_->BelongsToCurrentThread());
  DCHECK(component_ready_);
  base::PostTaskWithTraits(
      FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
      base::BindOnce(&Assets::LoadAssetsTask, task_runner, component_version_,
                     component_install_dir_, std::move(on_loaded)));
}

}  // namespace vr
