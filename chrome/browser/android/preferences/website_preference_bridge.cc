// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/preferences/website_preference_bridge.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browsing_data/browsing_data_local_storage_helper.h"
#include "chrome/browser/content_settings/cookie_settings.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "jni/WebsitePreferenceBridge_jni.h"
#include "storage/browser/quota/quota_client.h"
#include "storage/browser/quota/quota_manager.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;

static HostContentSettingsMap* GetHostContentSettingsMap() {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  return profile->GetHostContentSettingsMap();
}

static void GetOrigins(JNIEnv* env,
                       ContentSettingsType content_type,
                       jobject list,
                       jboolean managedOnly) {
  ContentSettingsForOneType all_settings;
  HostContentSettingsMap* content_settings_map = GetHostContentSettingsMap();
  content_settings_map->GetSettingsForOneType(
      content_type, std::string(), &all_settings);
  ContentSetting default_content_setting = content_settings_map->
      GetDefaultContentSetting(content_type, NULL);
  // Now add all origins that have a non-default setting to the list.
  for (const auto& settings_it : all_settings) {
    if (settings_it.setting == default_content_setting)
      continue;
    if (managedOnly &&
        HostContentSettingsMap::GetProviderTypeFromSource(settings_it.source) !=
            HostContentSettingsMap::ProviderType::POLICY_PROVIDER) {
      continue;
    }
    const std::string origin = settings_it.primary_pattern.ToString();
    const std::string embedder = settings_it.secondary_pattern.ToString();
    ScopedJavaLocalRef<jstring> jorigin = ConvertUTF8ToJavaString(env, origin);
    ScopedJavaLocalRef<jstring> jembedder;
    if (embedder != origin)
      jembedder = ConvertUTF8ToJavaString(env, embedder);
    switch (content_type) {
      case CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC:
      case CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA:
        Java_WebsitePreferenceBridge_insertVoiceAndVideoCaptureInfoIntoList(
            env, list, jorigin.obj(), jembedder.obj());
        break;
      case CONTENT_SETTINGS_TYPE_GEOLOCATION:
        Java_WebsitePreferenceBridge_insertGeolocationInfoIntoList(
            env, list, jorigin.obj(), jembedder.obj());
        break;
      case CONTENT_SETTINGS_TYPE_MIDI_SYSEX:
        Java_WebsitePreferenceBridge_insertMidiInfoIntoList(
            env, list, jorigin.obj(), jembedder.obj());
        break;
      case CONTENT_SETTINGS_TYPE_PROTECTED_MEDIA_IDENTIFIER:
        Java_WebsitePreferenceBridge_insertProtectedMediaIdentifierInfoIntoList(
            env, list, jorigin.obj(), jembedder.obj());
        break;
      case CONTENT_SETTINGS_TYPE_NOTIFICATIONS:
        Java_WebsitePreferenceBridge_insertPushNotificationIntoList(
            env, list, jorigin.obj(), jembedder.obj());
        break;
      default:
        DCHECK(false);
        break;
    }
  }
}

static jint GetSettingForOrigin(JNIEnv* env,
                                ContentSettingsType content_type,
                                jstring origin,
                                jstring embedder) {
  GURL url(ConvertJavaStringToUTF8(env, origin));
  GURL embedder_url(ConvertJavaStringToUTF8(env, embedder));
  ContentSetting setting = GetHostContentSettingsMap()->GetContentSetting(
      url,
      embedder_url,
      content_type,
      std::string());
  return setting;
}

static void SetSettingForOrigin(JNIEnv* env,
                                ContentSettingsType content_type,
                                jstring origin,
                                ContentSettingsPattern secondary_pattern,
                                jint value) {
  GURL url(ConvertJavaStringToUTF8(env, origin));
  ContentSetting setting = CONTENT_SETTING_DEFAULT;
  switch (value) {
    case -1: break;
    case 1: setting = CONTENT_SETTING_ALLOW; break;
    case 2: setting = CONTENT_SETTING_BLOCK; break;
    default:
      NOTREACHED();
  }
  GetHostContentSettingsMap()->SetContentSetting(
      ContentSettingsPattern::FromURLNoWildcard(url),
      secondary_pattern,
      content_type,
      std::string(),
      setting);
}

static void GetGeolocationOrigins(JNIEnv* env,
                                  jclass clazz,
                                  jobject list,
                                  jboolean managedOnly) {
  GetOrigins(env, CONTENT_SETTINGS_TYPE_GEOLOCATION, list, managedOnly);
}

static jint GetGeolocationSettingForOrigin(JNIEnv* env, jclass clazz,
    jstring origin, jstring embedder) {
  return GetSettingForOrigin(env, CONTENT_SETTINGS_TYPE_GEOLOCATION,
      origin, embedder);
}

static void SetGeolocationSettingForOrigin(JNIEnv* env, jclass clazz,
    jstring origin, jstring embedder, jint value) {
  GURL embedder_url(ConvertJavaStringToUTF8(env, embedder));
  SetSettingForOrigin(env, CONTENT_SETTINGS_TYPE_GEOLOCATION,
      origin, ContentSettingsPattern::FromURLNoWildcard(embedder_url), value);
}

static void GetMidiOrigins(JNIEnv* env, jclass clazz, jobject list) {
  GetOrigins(env, CONTENT_SETTINGS_TYPE_MIDI_SYSEX, list, false);
}

static jint GetMidiSettingForOrigin(JNIEnv* env, jclass clazz, jstring origin,
    jstring embedder) {
  return GetSettingForOrigin(env, CONTENT_SETTINGS_TYPE_MIDI_SYSEX, origin,
      embedder);
}

static void SetMidiSettingForOrigin(JNIEnv* env, jclass clazz, jstring origin,
    jstring embedder, jint value) {
  GURL embedder_url(ConvertJavaStringToUTF8(env, embedder));
  SetSettingForOrigin(env, CONTENT_SETTINGS_TYPE_MIDI_SYSEX, origin,
      ContentSettingsPattern::FromURLNoWildcard(embedder_url), value);
}

static void GetProtectedMediaIdentifierOrigins(JNIEnv* env, jclass clazz,
    jobject list) {
  GetOrigins(env, CONTENT_SETTINGS_TYPE_PROTECTED_MEDIA_IDENTIFIER, list,
             false);
}

static jint GetProtectedMediaIdentifierSettingForOrigin(JNIEnv* env,
    jclass clazz, jstring origin, jstring embedder) {
  return GetSettingForOrigin(
      env, CONTENT_SETTINGS_TYPE_PROTECTED_MEDIA_IDENTIFIER, origin, embedder);
}

static void SetProtectedMediaIdentifierSettingForOrigin(JNIEnv* env,
    jclass clazz, jstring origin, jstring embedder, jint value) {
  GURL embedder_url(ConvertJavaStringToUTF8(env, embedder));
  SetSettingForOrigin(env, CONTENT_SETTINGS_TYPE_PROTECTED_MEDIA_IDENTIFIER,
      origin, ContentSettingsPattern::FromURLNoWildcard(embedder_url), value);
}

static void GetPushNotificationOrigins(JNIEnv* env,
                                       jclass clazz,
                                       jobject list) {
  GetOrigins(env, CONTENT_SETTINGS_TYPE_NOTIFICATIONS, list, false);
}

static jint GetPushNotificationSettingForOrigin(JNIEnv* env, jclass clazz,
    jstring origin, jstring embedder) {
  return GetSettingForOrigin(env, CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
      origin, embedder);
}

static void SetPushNotificationSettingForOrigin(JNIEnv* env, jclass clazz,
    jstring origin, jstring embedder, jint value) {
  GURL embedder_url(ConvertJavaStringToUTF8(env, embedder));
  SetSettingForOrigin(env, CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
      origin, ContentSettingsPattern::FromURLNoWildcard(embedder_url), value);
}

static void GetVoiceAndVideoCaptureOrigins(JNIEnv* env,
                                           jclass clazz,
                                           jobject list,
                                           jboolean managedOnly) {
  GetOrigins(env, CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC, list, managedOnly);
  GetOrigins(env, CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA, list, managedOnly);
}

static jint GetVoiceCaptureSettingForOrigin(JNIEnv* env, jclass clazz,
    jstring origin, jstring embedder) {
  return GetSettingForOrigin(env, CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC,
      origin, embedder);
}

static jint GetVideoCaptureSettingForOrigin(JNIEnv* env, jclass clazz,
    jstring origin, jstring embedder) {
  return GetSettingForOrigin(env, CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA,
      origin, embedder);
}

static void SetVoiceCaptureSettingForOrigin(JNIEnv* env, jclass clazz,
    jstring origin, jstring embedder, jint value) {
  SetSettingForOrigin(env, CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC,
      origin, ContentSettingsPattern::Wildcard(), value);
}

static void SetVideoCaptureSettingForOrigin(JNIEnv* env, jclass clazz,
    jstring origin, jstring embedder, jint value) {
  SetSettingForOrigin(env, CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA,
      origin, ContentSettingsPattern::Wildcard(), value);
}

static scoped_refptr<CookieSettings> GetCookieSettings() {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  return CookieSettings::Factory::GetForProfile(profile);
}

static void GetCookieOrigins(JNIEnv* env,
                             jclass clazz,
                             jobject list,
                             jboolean managedOnly) {
  ContentSettingsForOneType all_settings;
  GetCookieSettings()->GetCookieSettings(&all_settings);
  const ContentSetting default_setting =
      GetCookieSettings()->GetDefaultCookieSetting(nullptr);
  for (const auto& settings_it : all_settings) {
    if (settings_it.setting == default_setting)
      continue;
    if (managedOnly &&
        HostContentSettingsMap::GetProviderTypeFromSource(settings_it.source) !=
            HostContentSettingsMap::ProviderType::POLICY_PROVIDER) {
      continue;
    }
    const std::string& origin = settings_it.primary_pattern.ToString();
    const std::string& embedder = settings_it.secondary_pattern.ToString();
    ScopedJavaLocalRef<jstring> jorigin = ConvertUTF8ToJavaString(env, origin);
    ScopedJavaLocalRef<jstring> jembedder;
    if (embedder != origin)
      jembedder = ConvertUTF8ToJavaString(env, embedder);
    Java_WebsitePreferenceBridge_insertCookieInfoIntoList(env, list,
        jorigin.obj(), jembedder.obj());
  }
}

static jint GetCookieSettingForOrigin(JNIEnv* env, jclass clazz,
    jstring origin, jstring embedder) {
  return GetSettingForOrigin(env, CONTENT_SETTINGS_TYPE_COOKIES, origin,
      embedder);
}

static void SetCookieSettingForOrigin(JNIEnv* env, jclass clazz,
    jstring origin, jstring embedder, jint value) {
  GURL url(ConvertJavaStringToUTF8(env, origin));
  ContentSettingsPattern primary_pattern(
      ContentSettingsPattern::FromURLNoWildcard(url));
  ContentSettingsPattern secondary_pattern(ContentSettingsPattern::Wildcard());
  if (value == -1) {
    GetCookieSettings()->ResetCookieSetting(primary_pattern, secondary_pattern);
  } else {
    GetCookieSettings()->SetCookieSetting(primary_pattern, secondary_pattern,
        value ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);
  }
}

namespace {

class StorageInfoFetcher :
      public base::RefCountedThreadSafe<StorageInfoFetcher> {
 public:
  StorageInfoFetcher(storage::QuotaManager* quota_manager,
                     const JavaRef<jobject>& java_callback)
      : env_(base::android::AttachCurrentThread()),
        quota_manager_(quota_manager),
        java_callback_(java_callback) {
  }

  void Run() {
    // QuotaManager must be called on IO thread, but java_callback must then be
    // called back on UI thread.
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&StorageInfoFetcher::GetUsageInfo, this));
  }

 protected:
  virtual ~StorageInfoFetcher() {}

 private:
  friend class base::RefCountedThreadSafe<StorageInfoFetcher>;

  void GetUsageInfo() {
    // We will have no explicit owner as soon as we leave this method.
    AddRef();
    quota_manager_->GetUsageInfo(
        base::Bind(&StorageInfoFetcher::OnGetUsageInfo, this));
  }

  void OnGetUsageInfo(const storage::UsageInfoEntries& entries) {
    entries_.insert(entries_.begin(), entries.begin(), entries.end());
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&StorageInfoFetcher::InvokeCallback, this));
    Release();
  }

  void InvokeCallback() {
    ScopedJavaLocalRef<jobject> list =
        Java_WebsitePreferenceBridge_createStorageInfoList(env_);

    storage::UsageInfoEntries::const_iterator i;
    for (i = entries_.begin(); i != entries_.end(); ++i) {
      if (i->usage <= 0) continue;
      ScopedJavaLocalRef<jstring> host =
          ConvertUTF8ToJavaString(env_, i->host);

      Java_WebsitePreferenceBridge_insertStorageInfoIntoList(
          env_, list.obj(), host.obj(), i->type, i->usage);
    }
    Java_StorageInfoReadyCallback_onStorageInfoReady(
        env_, java_callback_.obj(), list.obj());
  }

  JNIEnv* env_;
  storage::QuotaManager* quota_manager_;
  ScopedJavaGlobalRef<jobject> java_callback_;
  storage::UsageInfoEntries entries_;

  DISALLOW_COPY_AND_ASSIGN(StorageInfoFetcher);
};

class StorageDataDeleter :
      public base::RefCountedThreadSafe<StorageDataDeleter> {
 public:
  StorageDataDeleter(storage::QuotaManager* quota_manager,
                     const std::string& host,
                     storage::StorageType type,
                     const JavaRef<jobject>& java_callback)
      : env_(base::android::AttachCurrentThread()),
        quota_manager_(quota_manager),
        host_(host),
        type_(type),
        java_callback_(java_callback) {
  }

  void Run() {
    // QuotaManager must be called on IO thread, but java_callback must then be
    // called back on UI thread.  Grant ourself an extra reference to avoid
    // being deleted after DeleteHostData will return.
    AddRef();
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&storage::QuotaManager::DeleteHostData,
                   quota_manager_,
                   host_,
                   type_,
                   storage::QuotaClient::kAllClientsMask,
                   base::Bind(&StorageDataDeleter::OnHostDataDeleted,
                              this)));
  }

 protected:
  virtual ~StorageDataDeleter() {}

 private:
  friend class base::RefCountedThreadSafe<StorageDataDeleter>;

  void OnHostDataDeleted(storage::QuotaStatusCode) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
    quota_manager_->ResetUsageTracker(type_);
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&StorageDataDeleter::InvokeCallback, this));
    Release();
  }

  void InvokeCallback() {
    Java_StorageInfoClearedCallback_onStorageInfoCleared(
        env_, java_callback_.obj());
  }

  JNIEnv* env_;
  storage::QuotaManager* quota_manager_;
  std::string host_;
  storage::StorageType type_;
  ScopedJavaGlobalRef<jobject> java_callback_;
};

class LocalStorageInfoReadyCallback {
 public:
  LocalStorageInfoReadyCallback(
      const ScopedJavaLocalRef<jobject>& java_callback)
      : env_(base::android::AttachCurrentThread()),
        java_callback_(java_callback) {
  }

  void OnLocalStorageModelInfoLoaded(
      const std::list<BrowsingDataLocalStorageHelper::LocalStorageInfo>&
          local_storage_info) {
    ScopedJavaLocalRef<jobject> map =
        Java_WebsitePreferenceBridge_createLocalStorageInfoMap(env_);

    std::list<BrowsingDataLocalStorageHelper::LocalStorageInfo>::const_iterator
        i;
    for (i = local_storage_info.begin(); i != local_storage_info.end(); ++i) {
      ScopedJavaLocalRef<jstring> full_origin =
          ConvertUTF8ToJavaString(env_, i->origin_url.spec());
      ScopedJavaLocalRef<jstring> origin =
          ConvertUTF8ToJavaString(env_, i->origin_url.GetOrigin().spec());
      Java_WebsitePreferenceBridge_insertLocalStorageInfoIntoMap(
          env_, map.obj(), origin.obj(), full_origin.obj(), i->size);
    }

    Java_LocalStorageInfoReadyCallback_onLocalStorageInfoReady(
        env_, java_callback_.obj(), map.obj());
    delete this;
  }

 private:
  JNIEnv* env_;
  ScopedJavaGlobalRef<jobject> java_callback_;
};

}  // anonymous namespace

// TODO(jknotten): These methods should not be static. Instead we should
// expose a class to Java so that the fetch requests can be cancelled,
// and manage the lifetimes of the callback (and indirectly the helper
// by having a reference to it).

// The helper methods (StartFetching, DeleteLocalStorageFile, DeleteDatabase)
// are asynchronous. A "use after free" error is not possible because the
// helpers keep a reference to themselves for the duration of their tasks,
// which includes callback invocation.

static void FetchLocalStorageInfo(JNIEnv* env, jclass clazz,
    jobject java_callback) {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  scoped_refptr<BrowsingDataLocalStorageHelper> local_storage_helper(
      new BrowsingDataLocalStorageHelper(profile));
  // local_storage_callback will delete itself when it is run.
  LocalStorageInfoReadyCallback* local_storage_callback =
      new LocalStorageInfoReadyCallback(
          ScopedJavaLocalRef<jobject>(env, java_callback));
  local_storage_helper->StartFetching(
      base::Bind(&LocalStorageInfoReadyCallback::OnLocalStorageModelInfoLoaded,
                 base::Unretained(local_storage_callback)));
}

static void FetchStorageInfo(JNIEnv* env, jclass clazz, jobject java_callback) {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  scoped_refptr<StorageInfoFetcher> storage_info_fetcher(new StorageInfoFetcher(
      content::BrowserContext::GetDefaultStoragePartition(
          profile)->GetQuotaManager(),
      ScopedJavaLocalRef<jobject>(env, java_callback)));
  storage_info_fetcher->Run();
}

static void ClearLocalStorageData(JNIEnv* env, jclass clazz, jstring jorigin) {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  scoped_refptr<BrowsingDataLocalStorageHelper> local_storage_helper =
      new BrowsingDataLocalStorageHelper(profile);
  GURL origin_url = GURL(ConvertJavaStringToUTF8(env, jorigin));
  local_storage_helper->DeleteOrigin(origin_url);
}

static void ClearStorageData(JNIEnv* env,
                             jclass clazz,
                             jstring jhost,
                             jint type,
                             jobject java_callback) {
  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::string host = ConvertJavaStringToUTF8(env, jhost);
  scoped_refptr<StorageDataDeleter> storage_data_deleter(new StorageDataDeleter(
      content::BrowserContext::GetDefaultStoragePartition(
          profile)->GetQuotaManager(),
      host,
      static_cast<storage::StorageType>(type),
      ScopedJavaLocalRef<jobject>(env, java_callback)));
  storage_data_deleter->Run();
}

// Register native methods
bool RegisterWebsitePreferenceBridge(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
