// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_BOOKMARKS_PARTNER_BOOKMARKS_READER_H_
#define CHROME_BROWSER_ANDROID_BOOKMARKS_PARTNER_BOOKMARKS_READER_H_

#include <stdint.h>

#include <memory>

#include "base/android/jni_weak_ref.h"
#include "base/macros.h"
#include "components/bookmarks/browser/bookmark_model.h"

namespace favicon {
class LargeIconService;
}

class PartnerBookmarksShim;
class Profile;

#define PARTNER_BOOKMARKS_MINIMUM_FAVICON_SIZE_PX 16

namespace partnerbookmarks {

// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.partnerbookmarks
enum class FaviconFetchResult {
  // Successfully fetched a favicon from cache or server.
  SUCCESS = 0,
  // Received a server error fetching the favicon.
  FAILURE_SERVER_ERROR = 1,
  // The icon service was unavailable.
  FAILURE_ICON_SERVICE_UNAVAILABLE = 2,
  // There was nothing in the cache, but we opted out of retrieving from server.
  FAILURE_NOT_IN_CACHE = 3,
};

using FaviconFetchedCallback = base::OnceCallback<void(FaviconFetchResult)>;

}  // namespace partnerbookmarks

// Generates a partner bookmark hierarchy and handles submitting the results to
// the global PartnerBookmarksShim.
class PartnerBookmarksReader {
 public:
  PartnerBookmarksReader(PartnerBookmarksShim* partner_bookmarks_shim,
                         Profile* profile);
  ~PartnerBookmarksReader();

  // JNI methods
  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void Reset(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  jlong AddPartnerBookmark(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& jurl,
      const base::android::JavaParamRef<jstring>& jtitle,
      jboolean is_folder,
      jlong parent_id,
      const base::android::JavaParamRef<jbyteArray>& favicon,
      const base::android::JavaParamRef<jbyteArray>& touchicon,
      jboolean fetch_uncached_favicons_from_server,
      // Callback<FaviconFetchResult>
      const base::android::JavaParamRef<jobject>& j_callback);
  void PartnerBookmarksCreationComplete(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);

  favicon::LargeIconService* large_icon_service();
  void GetFavicon(const GURL& page_url,
                  Profile* profile,
                  bool fallback_to_server,
                  partnerbookmarks::FaviconFetchedCallback callback);
  void GetFaviconImpl(const GURL& page_url,
                      Profile* profile,
                      bool fallback_to_server,
                      partnerbookmarks::FaviconFetchedCallback callback);

 private:
  void GetFaviconFromCacheOrServer(
      const GURL& page_url,
      bool fallback_to_server,
      partnerbookmarks::FaviconFetchedCallback callback);
  void OnGetFaviconFromCacheFinished(
      const GURL& page_url,
      partnerbookmarks::FaviconFetchedCallback callback,
      bool fallback_to_server,
      const favicon_base::LargeIconResult& result);
  void OnGetFaviconFromServerFinished(
      const GURL& page_url,
      partnerbookmarks::FaviconFetchedCallback callback,
      favicon_base::GoogleFaviconServerRequestStatus status);

  PartnerBookmarksShim* partner_bookmarks_shim_;
  Profile* profile_;

  favicon::LargeIconService* large_icon_service_;
  base::CancelableTaskTracker favicon_task_tracker_;

  // JNI
  std::unique_ptr<bookmarks::BookmarkNode> wip_partner_bookmarks_root_;
  int64_t wip_next_available_id_;

  DISALLOW_COPY_AND_ASSIGN(PartnerBookmarksReader);
};

#endif  // CHROME_BROWSER_ANDROID_BOOKMARKS_PARTNER_BOOKMARKS_READER_H_
