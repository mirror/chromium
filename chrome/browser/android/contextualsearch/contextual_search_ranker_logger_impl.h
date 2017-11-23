// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_CONTEXTUALSEARCH_CONTEXTUAL_SEARCH_RANKER_LOGGER_IMPL_H_
#define CHROME_BROWSER_ANDROID_CONTEXTUALSEARCH_CONTEXTUAL_SEARCH_RANKER_LOGGER_IMPL_H_

#include "base/android/jni_android.h"
#include "base/memory/weak_ptr.h"
#include "url/gurl.h"

class ContextualSearchFieldTrial;

namespace base {
struct Feature;
}

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace assist_ranker {
class BinaryClassifierPredictor;
class RankerExample;
}  // namespace assist_ranker

// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.contextualsearch
enum AssistRankerPrediction {
  ASSIST_RANKER_PREDICTION_UNDETERMINED,
  ASSIST_RANKER_PREDICTION_UNAVAILABLE,
  ASSIST_RANKER_PREDICTION_SUPPRESS,
  ASSIST_RANKER_PREDICTION_SHOW,
};

// The Feature that queries the Contextual Search Ranker model (and has a URL).
extern const base::Feature kContextualSearchRankerQuery;

// Runs Ranker inference and logs features for Ranker model development.
// This is used to prediction whether a tap gesture will be useful to the user
// or not.
class ContextualSearchRankerLoggerImpl {
 public:
  ContextualSearchRankerLoggerImpl(JNIEnv* env, jobject obj);
  ~ContextualSearchRankerLoggerImpl();

  // Calls the destructor.  Should be called when this native object is no
  // longer needed.
  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

  // Sets up the logging and Ranker for Contextual Search features using the
  // given details.
  // |java_web_contents| is the |WebContents| of the base-page (where the user
  // tapped).
  void SetupLoggingAndRanker(
      JNIEnv* env,
      jobject obj,
      const base::android::JavaParamRef<jobject>& java_web_contents);

  // Logs a long value with the given feature name.
  void LogLong(JNIEnv* env,
               jobject obj,
               const base::android::JavaParamRef<jstring>& j_feature,
               jlong j_long);

  // Runs the model and returns the inference result as an
  // AssistRankerPrediction enum.
  AssistRankerPrediction RunInference(JNIEnv* env, jobject obj);

  // Writes the currently logged data and resets the current builder to be
  // ready to start logging the next set of data.
  void WriteLogAndReset(JNIEnv* env, jobject obj);

 private:
  // Adds feature to the RankerExample.
  void LogToExample(const std::string& feature_name, int value);

  // Sets up the Ranker Predictor for the given |web_contents|.
  void SetupRankerPredictor(content::WebContents* web_contents);

  // Whether Ranker predicting and model loading is enabled.
  bool IsRankerEnabled();

  // The field trial helper instance, always set up by the constructor.
  std::unique_ptr<ContextualSearchFieldTrial> field_trial_;

  // The Ranker Predictor for whether a tap gesture should be suppressed or not.
  base::WeakPtr<assist_ranker::BinaryClassifierPredictor> predictor_;

  // The |BrowserContext| currently associated with the above predictor.
  content::BrowserContext* browser_context_;

  // The current RankerExample or null.
  // Set of features from one example of a Tap to predict a suppression
  // decision.
  std::unique_ptr<assist_ranker::RankerExample> ranker_example_;

  // URL of the current page.
  GURL page_url_;

  // Whether Ranker has predicted the decision yet.
  bool has_predicted_decision_;

  // The linked Java object.
  base::android::ScopedJavaGlobalRef<jobject> java_object_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSearchRankerLoggerImpl);
};

#endif  // CHROME_BROWSER_ANDROID_CONTEXTUALSEARCH_CONTEXTUAL_SEARCH_RANKER_LOGGER_IMPL_H_
