// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/wallpaper/arc_wallpaper_service.h"

#include <stdlib.h>

#include <utility>

#include "ash/shell.h"
#include "ash/wallpaper/wallpaper_controller.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/chromeos/login/users/wallpaper/wallpaper_manager.h"
#include "chrome/browser/ui/ash/wallpaper_controller_client.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/signin/core/account_id/account_id.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "components/wallpaper/wallpaper_files_id.h"
#include "content/public/browser/browser_thread.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_util.h"

using user_manager::UserManager;

namespace arc {
namespace {

constexpr char kAndroidWallpaperFilename[] = "android.jpg";

// The structure must not go out from a method invoked on BrowserThread::UI
// because the primary account referenced by the class will be released.
struct PrimaryAccount {
  const AccountId& id;
  const bool is_active;
};

PrimaryAccount GetPrimaryAccount() {
  UserManager* const user_manager = UserManager::Get();
  const user_manager::User* const primary_user = user_manager->GetPrimaryUser();
  DCHECK(primary_user);
  const AccountId& account_id = primary_user->GetAccountId();
  return {account_id,
          account_id == user_manager->GetActiveUser()->GetAccountId()};
}

std::vector<uint8_t> EncodeImagePng(const gfx::ImageSkia image) {
  std::vector<uint8_t> result;
  gfx::PNGCodec::FastEncodeBGRASkBitmap(*image.bitmap(), true, &result);
  return result;
}

ash::WallpaperController* GetWallpaperController() {
  if (!ash::Shell::HasInstance())
    return nullptr;
  return ash::Shell::Get()->wallpaper_controller();
}

// Singleton factory for ArcWallpaperService.
class ArcWallpaperServiceFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcWallpaperService,
          ArcWallpaperServiceFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcWallpaperServiceFactory";

  static ArcWallpaperServiceFactory* GetInstance() {
    return base::Singleton<ArcWallpaperServiceFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcWallpaperServiceFactory>;
  ArcWallpaperServiceFactory() = default;
  ~ArcWallpaperServiceFactory() override = default;
};

class DecodeRequestSenderImpl
    : public ArcWallpaperService::DecodeRequestSender {
 public:
  void SendDecodeRequest(ImageDecoder::ImageRequest* request,
                         const std::vector<uint8_t>& data) override {
    ImageDecoder::StartWithOptions(request, data, ImageDecoder::DEFAULT_CODEC,
                                   true, gfx::Size());
  }
};

}  // namespace

class ArcWallpaperService::DecodeRequest : public ImageDecoder::ImageRequest {
 public:
  DecodeRequest(ArcWallpaperService* service, int32_t android_id)
      : service_(service), android_id_(android_id) {}
  ~DecodeRequest() override = default;

  // ImageDecoder::ImageRequest overrides.
  void OnImageDecoded(const SkBitmap& bitmap) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    // Make the SkBitmap immutable as we won't modify it. This is important
    // because otherwise it gets duplicated during painting, wasting memory.
    SkBitmap immutable_bitmap(bitmap);
    immutable_bitmap.setImmutable();
    gfx::ImageSkia image = gfx::ImageSkia::CreateFrom1xBitmap(immutable_bitmap);
    image.MakeThreadSafe();

    const PrimaryAccount& account = GetPrimaryAccount();
    wallpaper::WallpaperFilesId wallpaper_files_id =
        WallpaperControllerClient::Get()->GetFilesId(account.id);

    ash::WallpaperController* wallpaper_controller = GetWallpaperController();
    // TODO(crbug.com/776464): Replace |CanSetUserWallpaper| with mojo callback.
    if (wallpaper_controller &&
        !wallpaper_controller->CanSetUserWallpaper(
            account.id,
            !user_manager::UserManager::Get()->IsUserNonCryptohomeDataEphemeral(
                account.id) /* is_persistent */)) {
      // When kiosk app is running or policy is enforced, WallpaperController
      // doesn't process custom wallpaper requests.
      service_->NotifyWallpaperChangedAndReset(android_id_);
    } else {
      bool show_wallpaper = account.is_active;
      // When |show_wallpaper| is false, WallpaperController still saves custom
      // wallpaper for this user, but the wallpaper won't be shown right away.
      // TODO(crbug.com/618922): Allow specifying layout.
      WallpaperControllerClient::Get()->SetCustomWallpaper(
          account.id, wallpaper_files_id, kAndroidWallpaperFilename,
          wallpaper::WALLPAPER_LAYOUT_CENTER_CROPPED, wallpaper::CUSTOMIZED,
          image, show_wallpaper);

      if (show_wallpaper)
        service_->NotifyWallpaperChanged(android_id_);
      else
        service_->NotifyWallpaperChangedAndReset(android_id_);
    }

    // TODO(crbug.com/618922): Register the wallpaper to Chrome OS wallpaper
    // picker. Currently the new wallpaper does not appear there. The best way
    // to make this happen seems to do the same things as wallpaper_api.cc and
    // wallpaper_private_api.cc.
  }

  void OnDecodeImageFailed() override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    DLOG(ERROR) << "Failed to decode wallpaper image.";
    service_->NotifyWallpaperChangedAndReset(android_id_);
  }

 private:
  // ArcWallpaperService owns DecodeRequest, so it will outlive this.
  ArcWallpaperService* const service_;
  const int32_t android_id_;

  DISALLOW_COPY_AND_ASSIGN(DecodeRequest);
};

ArcWallpaperService::DecodeRequestSender::~DecodeRequestSender() = default;

void ArcWallpaperService::SetDecodeRequestSenderForTesting(
    std::unique_ptr<DecodeRequestSender> sender) {
  decode_request_sender_ = std::move(sender);
}

// static
ArcWallpaperService* ArcWallpaperService::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcWallpaperServiceFactory::GetForBrowserContext(context);
}

ArcWallpaperService::ArcWallpaperService(content::BrowserContext* context,
                                         ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service),
      decode_request_sender_(std::make_unique<DecodeRequestSenderImpl>()) {
  arc_bridge_service_->wallpaper()->SetHost(this);
}

ArcWallpaperService::~ArcWallpaperService() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  arc_bridge_service_->wallpaper()->SetHost(nullptr);
}

void ArcWallpaperService::SetWallpaper(const std::vector<uint8_t>& data,
                                       int32_t wallpaper_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (wallpaper_id == 0)
    wallpaper_id = -1;

  // Previous request will be cancelled at destructor of
  // ImageDecoder::ImageRequest.
  decode_request_ = std::make_unique<DecodeRequest>(this, wallpaper_id);
  DCHECK(decode_request_sender_);
  decode_request_sender_->SendDecodeRequest(decode_request_.get(), data);
}

void ArcWallpaperService::SetDefaultWallpaper() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // Previous request will be cancelled at destructor of
  // ImageDecoder::ImageRequest.
  decode_request_.reset();
  const PrimaryAccount& account = GetPrimaryAccount();
  WallpaperControllerClient::Get()->SetDefaultWallpaper(
      account.id, account.is_active /* show_wallpaper */);
}

void ArcWallpaperService::GetWallpaper(GetWallpaperCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  ash::WallpaperController* const wc = GetWallpaperController();
  DCHECK(wc);
  gfx::ImageSkia wallpaper = wc->GetWallpaper();
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&EncodeImagePng, wallpaper), std::move(callback));
}

void ArcWallpaperService::NotifyWallpaperChanged(int android_id) {
  mojom::WallpaperInstance* const wallpaper_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->wallpaper(),
                                  OnWallpaperChanged);
  if (wallpaper_instance == nullptr)
    return;
  wallpaper_instance->OnWallpaperChanged(android_id);
}

void ArcWallpaperService::NotifyWallpaperChangedAndReset(int32_t android_id) {
  // Invoke NotifyWallpaperChanged so that setWallpaper completes in Android
  // side.
  NotifyWallpaperChanged(android_id);
  // Invoke NotifyWallpaperChanged with -1 so that Android side regards the
  // wallpaper of |android_id_| is no longer used at Chrome side.
  NotifyWallpaperChanged(-1);
}

}  // namespace arc
