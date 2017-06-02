// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/protected_media_identifier_permission_context.h"

#include "base/command_line.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/string_split.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/permissions/permission_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "media/base/media_switches.h"
#include "net/base/url_util.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chromeos/chromeos_switches.h"
#endif

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
#error This file currently only supports Chrome OS and Android.
#endif

ProtectedMediaIdentifierPermissionContext::
    ProtectedMediaIdentifierPermissionContext(Profile* profile)
    : PermissionContextBase(profile,
                            CONTENT_SETTINGS_TYPE_PROTECTED_MEDIA_IDENTIFIER) {}

ProtectedMediaIdentifierPermissionContext::
    ~ProtectedMediaIdentifierPermissionContext() {
}

ContentSetting
ProtectedMediaIdentifierPermissionContext::GetPermissionStatusInternal(
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const GURL& embedding_origin) const {
  DVLOG(1) << __func__ << ": (" << requesting_origin.spec() << ", "
           << embedding_origin.spec() << ")";

  if (!requesting_origin.is_valid() || !embedding_origin.is_valid() ||
      !IsProtectedMediaIdentifierEnabled()) {
    return CONTENT_SETTING_BLOCK;
  }

  ContentSetting content_setting =
      PermissionContextBase::GetPermissionStatusInternal(
          render_frame_host, requesting_origin, embedding_origin);
  DCHECK(content_setting == CONTENT_SETTING_ALLOW ||
         content_setting == CONTENT_SETTING_BLOCK ||
         content_setting == CONTENT_SETTING_ASK);

  // For automated testing of protected content - having a prompt that
  // requires user intervention is problematic. If the domain has been
  // whitelisted as safe - suppress the request and allow.
  if (content_setting == CONTENT_SETTING_ASK &&
      IsOriginWhitelisted(requesting_origin)) {
    content_setting = CONTENT_SETTING_ALLOW;
  }

  return content_setting;
}

bool ProtectedMediaIdentifierPermissionContext::IsOriginWhitelisted(
    const GURL& origin) {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  if (command_line.GetSwitchValueASCII(switches::kUserDataDir).empty()) {
    return false;
  }

  const std::string whitelist = command_line.GetSwitchValueASCII(
      switches::kUnsafelyAllowProtectedMediaIdentifierForDomain);

  for (const std::string& domain : base::SplitString(
           whitelist, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    if (origin.DomainIs(domain)) {
      return true;
    }
  }

  return false;
}

void ProtectedMediaIdentifierPermissionContext::CancelPermissionRequest(
    content::WebContents* web_contents,
    const PermissionRequestID& id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  PermissionContextBase::CancelPermissionRequest(web_contents, id);
}

void ProtectedMediaIdentifierPermissionContext::UpdateTabContext(
    const PermissionRequestID& id,
    const GURL& requesting_frame,
    bool allowed) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // WebContents may have gone away.
  TabSpecificContentSettings* content_settings =
      TabSpecificContentSettings::GetForFrame(id.render_process_id(),
                                              id.render_frame_id());
  if (content_settings) {
    content_settings->OnProtectedMediaIdentifierPermissionSet(
        requesting_frame.GetOrigin(), allowed);
  }
}

bool ProtectedMediaIdentifierPermissionContext::IsRestrictedToSecureOrigins()
    const {
  // EME is not supported on insecure origins, see https://goo.gl/Ks5zf7
  // Note that origins whitelisted by --unsafely-treat-insecure-origin-as-secure
  // flag will be treated as "secure" so they will not be affected.
  return true;
}

// TODO(xhwang): We should consolidate the "protected content" related pref
// across platforms.
bool ProtectedMediaIdentifierPermissionContext::
    IsProtectedMediaIdentifierEnabled() const {
#if defined(OS_CHROMEOS)
  // Platform verification is not allowed in incognito or guest mode.
  if (profile()->IsOffTheRecord() || profile()->IsGuestSession()) {
    DVLOG(1) << "Protected media identifier disabled in incognito or guest "
                "mode.";
    return false;
  }

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(chromeos::switches::kSystemDevMode) &&
      !command_line->HasSwitch(chromeos::switches::kAllowRAInDevMode)) {
    DVLOG(1) << "Protected media identifier disabled in dev mode.";
    return false;
  }

  // This could be disabled by the device policy or by user's master switch.
  bool enabled_for_device = false;
  if (!chromeos::CrosSettings::Get()->GetBoolean(
          chromeos::kAttestationForContentProtectionEnabled,
          &enabled_for_device) ||
      !enabled_for_device ||
      !profile()->GetPrefs()->GetBoolean(prefs::kEnableDRM)) {
    DVLOG(1) << "Protected media identifier disabled by the user or by device "
                "policy.";
    return false;
  }
#endif

  return true;
}
