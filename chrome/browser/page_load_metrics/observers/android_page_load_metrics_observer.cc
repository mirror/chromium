// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/android_page_load_metrics_observer.h"

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/time/time.h"
#include "chrome/browser/net/nqe/ui_network_quality_estimator_service.h"
#include "chrome/browser/net/nqe/ui_network_quality_estimator_service_factory.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "jni/PageLoadMetrics_jni.h"
#include "url/gurl.h"

AndroidPageLoadMetricsObserver::AndroidPageLoadMetricsObserver(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  if (profile)
    network_quality_provider_ =
        UINetworkQualityEstimatorServiceFactory::GetForProfile(profile);
}

AndroidPageLoadMetricsObserver::ObservePolicy
AndroidPageLoadMetricsObserver::OnStart(
    content::NavigationHandle* navigation_handle,
    const GURL& currently_committed_url,
    bool started_in_foreground) {
  base::android::ScopedJavaLocalRef<jobject> java_web_contents =
      web_contents_->GetJavaWebContents();
  JNIEnv* env = base::android::AttachCurrentThread();
  if (network_quality_provider_) {
    Java_PageLoadMetrics_onNetworkQualityEstimate(
        env, java_web_contents,
        static_cast<jint>(
            network_quality_provider_->GetEffectiveConnectionType()),
        static_cast<jlong>(
            network_quality_provider_->GetHttpRTT()->InMilliseconds()),
        static_cast<jlong>(
            network_quality_provider_->GetTransportRTT()->InMilliseconds()));
  } else {
    Java_PageLoadMetrics_onNetworkQualityEstimate(
        env, java_web_contents,
        static_cast<jint>(net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN),
        static_cast<jlong>(0), static_cast<jlong>(0));
  }
  return CONTINUE_OBSERVING;
}

void AndroidPageLoadMetricsObserver::OnFirstContentfulPaintInPage(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& extra_info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  int64_t first_contentful_paint_ms =
      timing.paint_timing->first_contentful_paint->InMilliseconds();
  base::android::ScopedJavaLocalRef<jobject> java_web_contents =
      web_contents_->GetJavaWebContents();
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PageLoadMetrics_onFirstContentfulPaint(
      env, java_web_contents,
      static_cast<jlong>(
          (extra_info.navigation_start - base::TimeTicks()).InMicroseconds()),
      static_cast<jlong>(first_contentful_paint_ms));
}

void AndroidPageLoadMetricsObserver::OnLoadEventStart(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  int64_t load_event_start_ms =
      timing.document_timing->load_event_start->InMilliseconds();
  base::android::ScopedJavaLocalRef<jobject> java_web_contents =
      web_contents_->GetJavaWebContents();
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PageLoadMetrics_onLoadEventStart(
      env, java_web_contents,
      static_cast<jlong>(
          (info.navigation_start - base::TimeTicks()).InMicroseconds()),
      static_cast<jlong>(load_event_start_ms));
}

void AndroidPageLoadMetricsObserver::OnLoadedResource(
    const page_load_metrics::ExtraRequestCompleteInfo&
        extra_request_complete_info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (extra_request_complete_info.resource_type ==
      content::RESOURCE_TYPE_MAIN_FRAME) {
    if (did_dispatch_on_main_resource_) {
      // We are defensive for the case of something strange happening and return
      // in order not to post multiple times.
      return;
    }
    did_dispatch_on_main_resource_ = true;

    base::android::ScopedJavaLocalRef<jobject> java_web_contents =
        web_contents_->GetJavaWebContents();
    JNIEnv* env = base::android::AttachCurrentThread();
    const net::LoadTimingInfo& timing =
        *extra_request_complete_info.load_timing_info;
    int64_t dns_start =
        timing.connect_timing.dns_start.since_origin().InMilliseconds();
    int64_t dns_end =
        timing.connect_timing.dns_end.since_origin().InMilliseconds();
    int64_t connect_start =
        timing.connect_timing.connect_start.since_origin().InMilliseconds();
    int64_t connect_end =
        timing.connect_timing.connect_end.since_origin().InMilliseconds();
    int64_t request_start =
        timing.request_start.since_origin().InMilliseconds();
    int64_t send_start = timing.send_start.since_origin().InMilliseconds();
    int64_t send_end = timing.send_end.since_origin().InMilliseconds();
    Java_PageLoadMetrics_onLoadedMainResource(
        env, java_web_contents, static_cast<jlong>(dns_start),
        static_cast<jlong>(dns_end), static_cast<jlong>(connect_start),
        static_cast<jlong>(connect_end), static_cast<jlong>(request_start),
        static_cast<jlong>(send_start), static_cast<jlong>(send_end));
  }
}
