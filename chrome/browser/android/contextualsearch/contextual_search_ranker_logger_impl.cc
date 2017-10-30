// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <chrome/browser/android/contextualsearch/contextual_search_ranker_logger_impl.h>

#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/metrics_hashes.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/android/contextualsearch/contextual_search_field_trial.h"
#include "chrome/browser/assist_ranker/assist_ranker_service_factory.h"
#include "chrome/browser/browser_process.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/machine_intelligence/assist_ranker_service_impl.h"
#include "components/machine_intelligence/binary_classifier_predictor.h"
#include "components/machine_intelligence/proto/ranker_example.pb.h"
#include "content/public/browser/web_contents.h"
#include "jni/ContextualSearchRankerLoggerImpl_jni.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace content {
class BrowserContext;
}

namespace {
// TODO(donnd): Check with Philippe to see if these should be field trial
// params, and update these defaults.
const char kContextualSearchModelUrl[] = "contextual_search_model";
const char kContextualSearchModelFilename[] = "contextual_search_model";
const char kContextualSearchUmaPrefix[] = "contextual_search_uma";

const char kContextualSearchRankerDidPredict[] = "OutcomeRankerDidPredict";
const char kContextualSearchRankerPrediction[] = "OutcomeRankerPrediction";

// TODO(donnd, hamelphi): move hex-hash-string to Ranker.
std::string HexHashFeatureName(std::string feature_name) {
  uint64_t feature_key = base::HashMetricName(feature_name);
  return base::StringPrintf("%016" PRIx64, feature_key);
}

}  // namespace

ContextualSearchRankerLoggerImpl::ContextualSearchRankerLoggerImpl(JNIEnv* env,
                                                                   jobject obj)
    : ukm_recorder_(nullptr), builder_(nullptr) {
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

  GURL page_url = web_contents->GetURL();
  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  SetUkmRecorder(ukm_recorder, page_url);

  // Set up the Ranker model.
  if (field_trial_->IsRankerIntegrationOrMlTapSuppressionEnabled()) {
    content::BrowserContext* browser_context =
        web_contents->GetBrowserContext();
    machine_intelligence::AssistRankerService* assist_ranker_service =
        assist_ranker::AssistRankerServiceFactory::GetForBrowserContext(
            browser_context);
    predictor_ = assist_ranker_service->FetchBinaryClassifierPredictor(
        GURL(kContextualSearchModelUrl), kContextualSearchModelFilename,
        kContextualSearchUmaPrefix);
    // Start building example data based on features to be gathered and logged.
    ranker_example_.reset(new machine_intelligence::RankerExample());
  }
}

void ContextualSearchRankerLoggerImpl::SetUkmRecorder(
    ukm::UkmRecorder* ukm_recorder,
    const GURL& page_url) {
  if (!ukm_recorder) {
    builder_.reset();
    return;
  }

  ukm_recorder_ = ukm_recorder;
  source_id_ = ukm_recorder_->GetNewSourceID();
  ukm_recorder_->UpdateSourceURL(source_id_, page_url);
  builder_ = ukm_recorder_->GetEntryBuilder(source_id_, "ContextualSearch");
}

void ContextualSearchRankerLoggerImpl::LogLong(
    JNIEnv* env,
    jobject obj,
    const base::android::JavaParamRef<jstring>& j_feature,
    jlong j_long) {
  if (!builder_)
    return;

  std::string feature = base::android::ConvertJavaStringToUTF8(env, j_feature);
  builder_->AddMetric(feature.c_str(), j_long);

  // Also write to Ranker if prediction of the decision has not been made yet.
  if (field_trial_->IsRankerIntegrationOrMlTapSuppressionEnabled() &&
      !has_predicted_decision_) {
    std::string hex_feature_key(HexHashFeatureName(feature));
    auto& features = *ranker_example_->mutable_features();
    features[hex_feature_key].set_int32_value(j_long);
  }
}

MachineIntelligencePrediction ContextualSearchRankerLoggerImpl::RunInference(
    JNIEnv* env,
    jobject obj) {
  has_predicted_decision_ = true;
  bool prediction = false;
  bool was_able_to_predict = false;
  if (field_trial_->IsRankerIntegrationOrMlTapSuppressionEnabled()) {
    bool was_able_to_predict =
        predictor_->Predict(*ranker_example_, &prediction);
    // Log to UMA whether we were able to predict or not.
    base::UmaHistogramBoolean("Search.ContextualSearchRankerWasAbleToPredict",
                              was_able_to_predict);
    // Log the Ranker decision to UKM, including whether we were able to make
    // any prediction.
    if (builder_) {
      builder_->AddMetric(kContextualSearchRankerDidPredict,
                          was_able_to_predict);
      // TODO(donnd, hamelphi): conditionalize on was_able_to_predict?
      builder_->AddMetric(kContextualSearchRankerPrediction, prediction);
    }
  }
  MachineIntelligencePrediction prediction_enum;
  if (was_able_to_predict) {
    if (prediction) {
      prediction_enum = MACHINE_INTELLIGENCE_PREDICTION_YES;
    } else {
      prediction_enum = MACHINE_INTELLIGENCE_PREDICTION_NO;
    }
  } else {
    prediction_enum = MACHINE_INTELLIGENCE_PREDICTION_UNAVAILABLE;
  }
  return prediction_enum;
}

void ContextualSearchRankerLoggerImpl::WriteLogAndReset(JNIEnv* env,
                                                        jobject obj) {
  has_predicted_decision_ = false;
  if (!ukm_recorder_)
    return;

  // Set up another builder for the next record (in case it's needed).
  builder_ = ukm_recorder_->GetEntryBuilder(source_id_, "ContextualSearch");
  ranker_example_.reset();
}

// Java wrapper boilerplate

void ContextualSearchRankerLoggerImpl::Destroy(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  delete this;
}

jlong Init(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj) {
  ContextualSearchRankerLoggerImpl* ranker_logger_impl =
      new ContextualSearchRankerLoggerImpl(env, obj);
  return reinterpret_cast<intptr_t>(ranker_logger_impl);
}
