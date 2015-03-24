// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/content_settings/core/common/pref_names.h"

namespace prefs {

// Version of the pattern format used to define content settings.
const char kContentSettingsVersion[] = "profile.content_settings.pref_version";

// Patterns for mapping origins to origin related settings. Default settings
// will be applied to origins that don't match any of the patterns. The pattern
// format used is defined by kContentSettingsVersion.
const char kContentSettingsPatternPairs[] =
    "profile.content_settings.pattern_pairs";

// Integer that specifies the index of the tab the user was on when they
// last visited the content settings window.
const char kContentSettingsWindowLastTabIndex[] =
    "content_settings_window.last_tab_index";

// Dictionary of content settings applied to all hosts by default.
// TODO(msramek): Deprecated. Use the individual preferences below.
const char kDefaultContentSettings[] = "profile.default_content_settings";

// Whether |kDefaultContentSettings| has already been migrated to
// |kDefault<type>Setting| prefs.
const char kMigratedDefaultContentSettings[] =
    "profile.migrated_default_content_settings";

// Preferences storing the default values for individual content settings.
const char kDefaultCookiesSetting[] =
    "profile.default_content_setting_values.cookies";
const char kDefaultImagesSetting[] =
    "profile.default_content_setting_values.images";
const char kDefaultJavaScriptSetting[] =
    "profile.default_content_setting_values.javascript";
const char kDefaultPluginsSetting[] =
    "profile.default_content_setting_values.plugins";
const char kDefaultPopupsSetting[] =
    "profile.default_content_setting_values.popups";
const char kDefaultGeolocationSetting[] =
    "profile.default_content_setting_values.geolocation";
const char kDefaultNotificationsSetting[] =
    "profile.default_content_setting_values.notifications";
const char kDefaultAutoSelectCertificateSetting[] =
    "profile.default_content_setting_values.auto_select_certificate";
const char kDefaultFullScreenSetting[] =
    "profile.default_content_setting_values.fullscreen";
const char kDefaultMouseLockSetting[] =
    "profile.default_content_setting_values.mouselock";
const char kDefaultMixedScriptSetting[] =
    "profile.default_content_setting_values.mixed_script";
const char kDefaultMediaStreamSetting[] =
    "profile.default_content_setting_values.media_stream";
const char kDefaultMediaStreamMicSetting[] =
    "profile.default_content_setting_values.media_stream_mic";
const char kDefaultMediaStreamCameraSetting[] =
    "profile.default_content_setting_values.media_stream_camera";
const char kDefaultProtocolHandlersSetting[] =
    "profile.default_content_setting_values.protocol_handlers";
const char kDefaultPpapiBrokerSetting[] =
    "profile.default_content_setting_values.ppapi_broker";
const char kDefaultAutomaticDownloadsSetting[] =
    "profile.default_content_setting_values.automatic_downloads";
const char kDefaultMidiSysexSetting[] =
    "profile.default_content_setting_values.midi_sysex";
const char kDefaultPushMessagingSetting[] =
    "profile.default_content_setting_values.push_messaging";
const char kDefaultSSLCertDecisionsSetting[] =
    "profile.default_content_setting_values.ssl_cert_decisions";
#if defined(OS_WIN)
const char kDefaultMetroSwitchToDesktopSetting[] =
    "profile.default_content_setting_values.metro_switch_to_desktop";
#elif defined(OS_ANDROID) || defined(OS_CHROMEOS)
const char kDefaultProtectedMediaIdentifierSetting[] =
    "profile.default_content_setting_values.protected_media_identifier";
#endif
const char kDefaultAppBannerSetting[] =
    "profile.default_content_setting_values.app_banner";

// Dictionary of content settings that can globally disallow all hosts by
// default. If a value is set, it means the setting is globally disallowed.
// If a value is not set, it means the setting is allowed.
const char kOverrideContentSettings[] = "profile.override_content_settings";

// Preferences that are exclusively used to store managed values for default
// content settings.
const char kManagedDefaultCookiesSetting[] =
    "profile.managed_default_content_settings.cookies";
const char kManagedDefaultImagesSetting[] =
    "profile.managed_default_content_settings.images";
const char kManagedDefaultJavaScriptSetting[] =
    "profile.managed_default_content_settings.javascript";
const char kManagedDefaultPluginsSetting[] =
    "profile.managed_default_content_settings.plugins";
const char kManagedDefaultPopupsSetting[] =
    "profile.managed_default_content_settings.popups";
const char kManagedDefaultGeolocationSetting[] =
    "profile.managed_default_content_settings.geolocation";
const char kManagedDefaultNotificationsSetting[] =
    "profile.managed_default_content_settings.notifications";
const char kManagedDefaultMediaStreamSetting[] =
    "profile.managed_default_content_settings.media_stream";

// Preferences that are exclusively used to store managed
// content settings patterns.
const char kManagedCookiesAllowedForUrls[] =
    "profile.managed_cookies_allowed_for_urls";
const char kManagedCookiesBlockedForUrls[] =
    "profile.managed_cookies_blocked_for_urls";
const char kManagedCookiesSessionOnlyForUrls[] =
    "profile.managed_cookies_sessiononly_for_urls";
const char kManagedImagesAllowedForUrls[] =
    "profile.managed_images_allowed_for_urls";
const char kManagedImagesBlockedForUrls[] =
    "profile.managed_images_blocked_for_urls";
const char kManagedJavaScriptAllowedForUrls[] =
    "profile.managed_javascript_allowed_for_urls";
const char kManagedJavaScriptBlockedForUrls[] =
    "profile.managed_javascript_blocked_for_urls";
const char kManagedPluginsAllowedForUrls[] =
    "profile.managed_plugins_allowed_for_urls";
const char kManagedPluginsBlockedForUrls[] =
    "profile.managed_plugins_blocked_for_urls";
const char kManagedPopupsAllowedForUrls[] =
    "profile.managed_popups_allowed_for_urls";
const char kManagedPopupsBlockedForUrls[] =
    "profile.managed_popups_blocked_for_urls";
const char kManagedNotificationsAllowedForUrls[] =
    "profile.managed_notifications_allowed_for_urls";
const char kManagedNotificationsBlockedForUrls[] =
    "profile.managed_notifications_blocked_for_urls";
const char kManagedAutoSelectCertificateForUrls[] =
    "profile.managed_auto_select_certificate_for_urls";

}  // namespace prefs
