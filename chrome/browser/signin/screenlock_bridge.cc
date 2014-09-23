// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/screenlock_bridge.h"

#include "base/logging.h"
#include "base/strings/string16.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/signin/core/browser/signin_manager.h"

#if defined(OS_CHROMEOS)
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#endif

namespace {

base::LazyInstance<ScreenlockBridge> g_screenlock_bridge_bridge_instance =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
ScreenlockBridge* ScreenlockBridge::Get() {
  return g_screenlock_bridge_bridge_instance.Pointer();
}

ScreenlockBridge::UserPodCustomIconOptions::UserPodCustomIconOptions()
    : width_(0u),
      height_(0u),
      animation_set_(false),
      animation_resource_width_(0u),
      animation_frame_length_ms_(0u),
      opacity_(100u),
      autoshow_tooltip_(false),
      hardlock_on_click_(false) {
}

ScreenlockBridge::UserPodCustomIconOptions::~UserPodCustomIconOptions() {}

scoped_ptr<base::DictionaryValue>
ScreenlockBridge::UserPodCustomIconOptions::ToDictionaryValue() const {
  scoped_ptr<base::DictionaryValue> result(new base::DictionaryValue());
  if (icon_resource_url_.empty())
    return result.Pass();

  result->SetString("resourceUrl", icon_resource_url_);

  if (!tooltip_.empty()) {
    base::DictionaryValue* tooltip_options = new base::DictionaryValue();
    tooltip_options->SetString("text", tooltip_);
    tooltip_options->SetBoolean("autoshow", autoshow_tooltip_);
    result->Set("tooltip", tooltip_options);
  }

  base::DictionaryValue* size = new base::DictionaryValue();
  size->SetInteger("height", height_);
  size->SetInteger("width", width_);
  result->Set("size", size);

  result->SetInteger("opacity", opacity_);

  if (animation_set_) {
    base::DictionaryValue* animation = new base::DictionaryValue();
    animation->SetInteger("resourceWidth",
                          animation_resource_width_);
    animation->SetInteger("frameLengthMs",
                          animation_frame_length_ms_);
    result->Set("animation", animation);
  }

  if (hardlock_on_click_)
    result->SetBoolean("hardlockOnClick", true);

  return result.Pass();
}

void ScreenlockBridge::UserPodCustomIconOptions::SetIconAsResourceURL(
    const std::string& url) {
  icon_resource_url_ = url;
}


void ScreenlockBridge::UserPodCustomIconOptions::SetSize(size_t icon_width,
                                                         size_t icon_height) {
  width_ = icon_width;
  height_ = icon_height;
}

void ScreenlockBridge::UserPodCustomIconOptions::SetAnimation(
    size_t resource_width,
    size_t frame_length_ms) {
  animation_set_ = true;
  animation_resource_width_ = resource_width;
  animation_frame_length_ms_ = frame_length_ms;
}

void ScreenlockBridge::UserPodCustomIconOptions::SetOpacity(size_t opacity) {
  DCHECK_LE(opacity, 100u);
  opacity_ = opacity;
}

void ScreenlockBridge::UserPodCustomIconOptions::SetTooltip(
    const base::string16& tooltip,
    bool autoshow) {
  tooltip_ = tooltip;
  autoshow_tooltip_ = autoshow;
}

void ScreenlockBridge::UserPodCustomIconOptions::SetHardlockOnClick() {
  hardlock_on_click_ = true;
}

// static
std::string ScreenlockBridge::GetAuthenticatedUserEmail(Profile* profile) {
  // |profile| has to be a signed-in profile with SigninManager already
  // created. Otherwise, just crash to collect stack.
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfileIfExists(profile);
  return signin_manager->GetAuthenticatedUsername();
}

ScreenlockBridge::ScreenlockBridge() : lock_handler_(NULL) {
}

ScreenlockBridge::~ScreenlockBridge() {
}

void ScreenlockBridge::SetLockHandler(LockHandler* lock_handler) {
  DCHECK(lock_handler_ == NULL || lock_handler == NULL);
  lock_handler_ = lock_handler;
  if (lock_handler_)
    FOR_EACH_OBSERVER(Observer, observers_, OnScreenDidLock());
  else
    FOR_EACH_OBSERVER(Observer, observers_, OnScreenDidUnlock());
}

void ScreenlockBridge::SetFocusedUser(const std::string& user_id) {
  if (user_id == focused_user_id_)
    return;
  focused_user_id_ = user_id;
  FOR_EACH_OBSERVER(Observer, observers_, OnFocusedUserChanged(user_id));
}

bool ScreenlockBridge::IsLocked() const {
  return lock_handler_ != NULL;
}

void ScreenlockBridge::Lock(Profile* profile) {
#if defined(OS_CHROMEOS)
  chromeos::SessionManagerClient* session_manager =
      chromeos::DBusThreadManager::Get()->GetSessionManagerClient();
  session_manager->RequestLockScreen();
#else
  profiles::LockProfile(profile);
#endif
}

void ScreenlockBridge::Unlock(Profile* profile) {
  if (lock_handler_)
    lock_handler_->Unlock(GetAuthenticatedUserEmail(profile));
}

void ScreenlockBridge::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ScreenlockBridge::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}
