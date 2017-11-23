// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <chrome/browser/android/contextualsearch/contextual_search_ranker_logger_impl.h>

#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/metrics_hashes.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/android/contextualsearch/contextual_search_field_trial.h"
#include "chrome/browser/assist_ranker/assist_ranker_service_factory.h"
#include "chrome/browser/browser_process.h"
#include "components/assist_ranker/assist_ranker_service_impl.h"
#include "components/assist_ranker/binary_classifier_predictor.h"
#include "components/assist_ranker/proto/ranker_example.pb.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/web_contents.h"
#include "jni/ContextualSearchRankerLoggerImpl_jni.h"

namespace content {
class BrowserContext;
}

const base::Feature kContextualSearchRankerQuery{
    "ContextualSearchRankerQuery", base::FEATURE_DISABLED_BY_DEFAULT};

namespace {

// const char kContextualSearchRankerModelUrlParamName[] =
//     "contextual-search-ranker-model-url";
const char kContextualSearchModelFilename[] = "contextual_search_model";

const char kContextualSearchRankerDidPredict[] = "OutcomeRankerDidPredict";
const char kContextualSearchRankerPrediction[] = "OutcomeRankerPrediction";

// const base::FeatureParam<std::string> kModelUrl{
//     &kContextualSearchRankerQuery, kContextualSearchRankerModelUrlParamName,
//     ""};

// TODO(donnd, hamelphi): move hex-hash-string to Ranker.
std::string HexHashFeatureName(const std::string& feature_name) {
  uint64_t feature_key = base::HashMetricName(feature_name);
  return base::StringPrintf("%016" PRIx64, feature_key);
}

}  // namespace

ContextualSearchRankerLoggerImpl::ContextualSearchRankerLoggerImpl(JNIEnv* env,
                                                                   jobject obj)
    : field_trial_(nullptr),
      predictor_(nullptr),
      browser_context_(nullptr),
      ranker_example_(nullptr),
      has_predicted_decision_(false),
      java_object_(nullptr) {
  java_object_.Reset(env, obj);
  // TODO(donnd): consider making a singleton CSFieldTrial instead of many?
  field_trial_.reset(new ContextualSearchFieldTrial());
}

ContextualSearchRankerLoggerImpl::~ContextualSearchRankerLoggerImpl() {
  java_object_ = nullptr;
}

void ContextualSearchRankerLoggerImpl::SetupLoggingAndRanker(
    JNIEnv* env,
    jobject obj,
    const base::android::JavaParamRef<jobject>& java_web_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(java_web_contents);
  if (!web_contents)
    return;

  page_url_ = web_contents->GetURL();

  if (IsRankerEnabled()) {
    SetupRankerPredictor(web_contents);
    // Start building example data based on features to be gathered and logged.
    ranker_example_.reset(new assist_ranker::RankerExample());
  }
}

void ContextualSearchRankerLoggerImpl::SetupRankerPredictor(
    content::WebContents* web_contents) {
  // Set up the Ranker predictor.
  if (IsRankerEnabled()) {
    // Create one predictor for the current BrowserContext.
    content::BrowserContext* browser_context =
        web_contents->GetBrowserContext();
    if (browser_context == browser_context_)
      return;

    browser_context_ = browser_context;
    assist_ranker::AssistRankerService* assist_ranker_service =
        assist_ranker::AssistRankerServiceFactory::GetForBrowserContext(
            browser_context);
    // FIXME: Get string from predictor config.
    predictor_ = assist_ranker_service->FetchBinaryClassifierPredictor(
        kContextualSearchModelFilename);
  }
}

void ContextualSearchRankerLoggerImpl::LogToExample(
    const std::string& feature_name,
    int value) {
  // FIXME move HexHashFeatureName to Ranker.
  std::string hex_feature_key(HexHashFeatureName(feature_name));
  auto& features = *ranker_example_->mutable_features();
  features[hex_feature_key].set_int32_value(value);
}

void ContextualSearchRankerLoggerImpl::LogLong(
    JNIEnv* env,
    jobject obj,
    const base::android::JavaParamRef<jstring>& j_feature,
    jlong j_long) {
  std::string feature = base::android::ConvertJavaStringToUTF8(env, j_feature);
  LogToExample(feature, j_long);
}

AssistRankerPrediction ContextualSearchRankerLoggerImpl::RunInference(
    JNIEnv* env,
    jobject obj) {
  has_predicted_decision_ = true;
  bool prediction = false;
  bool was_able_to_predict = false;
  if (IsRankerEnabled() && predictor_) {
    was_able_to_predict = predictor_->Predict(*ranker_example_, &prediction);
    // Log to UMA whether we were able to predict or not.
    base::UmaHistogramBoolean("Search.ContextualSearchRankerWasAbleToPredict",
                              was_able_to_predict);
    // FIXME: this could be logged internally by ranker.
    LogToExample(kContextualSearchRankerDidPredict,
                 static_cast<int>(was_able_to_predict));
    if (was_able_to_predict) {
      LogToExample(kContextualSearchRankerPrediction,
                   static_cast<int>(prediction));
    }
  }
  AssistRankerPrediction prediction_enum;
  if (was_able_to_predict) {
    if (prediction) {
      prediction_enum = ASSIST_RANKER_PREDICTION_SHOW;
    } else {
      prediction_enum = ASSIST_RANKER_PREDICTION_SUPPRESS;
    }
  } else {
    prediction_enum = ASSIST_RANKER_PREDICTION_UNAVAILABLE;
  }
  return prediction_enum;
}

void ContextualSearchRankerLoggerImpl::WriteLogAndReset(JNIEnv* env,
                                                        jobject obj) {
  has_predicted_decision_ = false;
  if (predictor_)
    // FIXME: Should ranker take care of resetting the example?
    predictor_->LogExample(*ranker_example_.get(), page_url_);
  ranker_example_.reset();
}

bool ContextualSearchRankerLoggerImpl::IsRankerEnabled() {
  return field_trial_->IsRankerIntegrationOrMlTapSuppressionEnabled() &&
         base::FeatureList::IsEnabled(kContextualSearchRankerQuery);
}

// Java wrapper boilerplate

void ContextualSearchRankerLoggerImpl::Destroy(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  delete this;
}

jlong JNI_ContextualSearchRankerLoggerImpl_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  ContextualSearchRankerLoggerImpl* ranker_logger_impl =
      new ContextualSearchRankerLoggerImpl(env, obj);
  return reinterpret_cast<intptr_t>(ranker_logger_impl);
}
