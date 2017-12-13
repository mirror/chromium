// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_CONTEXTUALSEARCH_CONTEXTUAL_SEARCH_MANAGER_H_
#define CHROME_BROWSER_ANDROID_CONTEXTUALSEARCH_CONTEXTUAL_SEARCH_MANAGER_H_

#include <stddef.h>

#include "base/macros.h"
#include "base/task/cancelable_task_tracker.h"
#include "chrome/browser/android/contextualsearch/contextual_search_context.h"
#include "chrome/browser/android/contextualsearch/contextual_search_delegate.h"
#include "components/contextual_search/browser/contextual_search_js_api_handler.h"

// Manages the native extraction and request logic for Contextual Search,
// and interacts with the Java ContextualSearchManager for UX.
// Most of the work is done by the associated ContextualSearchDelegate.
class ContextualSearchManager
    : public contextual_search::ContextualSearchJsApiHandler {
 public:
  // Constructs a native manager associated with the Java manager.
  ContextualSearchManager(JNIEnv* env,
                          const base::android::JavaRef<jobject>& obj);
  ~ContextualSearchManager() override;

  // Called by the Java ContextualSearchManager when it is being destroyed.
  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

  // Starts the request to get the search terms to use for the given selection,
  // by accessing our server with the content of the page (from the given
  // content view core object).
  // Any outstanding server requests are canceled.
  // When the server responds with the search term, the Java object is notified
  // by calling OnSearchTermResolutionResponse().
  void StartSearchTermResolutionRequest(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_contextual_search_context,
      const base::android::JavaParamRef<jobject>& j_base_web_contents);

  // Gathers the surrounding text around the selection and saves it locally.
  // Does not send a search term resolution request to the server.
  void GatherSurroundingText(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_contextual_search_context,
      const base::android::JavaParamRef<jobject>& j_base_web_contents);

  // Gets the target language for translation purposes.
  base::android::ScopedJavaLocalRef<jstring> GetTargetLanguage(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);

  // Gets the accept-languages preference string.
  base::android::ScopedJavaLocalRef<jstring> GetAcceptLanguages(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);

  // Allows the Contextual Search JS API to be enabled for the given
  // |WebContents|. Once this method is called the JS API can be enabled by
  // calling |EnableContextualSearchJsApiForUrl| before the page loads.
  void AllowContextualSearchJsApiForWebContents(
      JNIEnv* env,
      jobject obj,
      const base::android::JavaParamRef<jobject>& j_web_contents);

  // Enables the Contextual Search JS API for the given |j_url|.
  // This method should be called as soon as possible when the URL navigation
  // starts so the JavaScript API can be established before the page executes.
  // A call to AllowContextualSearchJsApiForWebContents must have been made
  // before making this call. The given URL is stored for future reference when
  // ShouldEnableJsApi is called by a Renderer through mojo.
  void EnableContextualSearchJsApiForUrl(
      JNIEnv* env,
      jobject obj,
      const base::android::JavaParamRef<jstring>& j_url);

  // ContextualSearchJsApiHandler overrides:
  void SetCaption(std::string caption, bool does_answer) override;

  // Determines whether the JS API should be enabled for the given URL.
  // Calls the given |callback| with the answer: whether the API should be
  // enabled.
  void ShouldEnableJsApi(
      const std::string& url,
      contextual_search::mojom::ContextualSearchJsApiService::
          ShouldEnableJsApiCallback callback) override;

 private:
  void OnSearchTermResolutionResponse(
      const ResolvedSearchTerm& resolved_search_term);

  // Calls back to Java with notification when a sample of text surrounding the
  // selection is available.
  void OnTextSurroundingSelectionAvailable(
      const std::string& encoding,
      const base::string16& surrounding_text,
      size_t start_offset,
      size_t end_offset);

  // Our global reference to the Java ContextualSearchManager.
  base::android::ScopedJavaGlobalRef<jobject> java_manager_;

  // The url of the overlay, used to determine if the JS API should be enabled.
  // See ShouldEnableJsApi.
  std::string overlay_url_;

  // The delegate we're using the do the real work.
  std::unique_ptr<ContextualSearchDelegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSearchManager);
};

#endif  // CHROME_BROWSER_ANDROID_CONTEXTUALSEARCH_CONTEXTUAL_SEARCH_MANAGER_H_
