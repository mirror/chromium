// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_DATA_REDUCTION_PROXY_PINGBACK_CLIENT_IMPL_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_DATA_REDUCTION_PROXY_PINGBACK_CLIENT_IMPL_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "base/sequence_checker.h"
#include "build/build_config.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_pingback_client.h"
#include "components/data_reduction_proxy/proto/pageload_metrics.pb.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

#if defined(OS_ANDROID)
#include "components/crash/content/browser/crash_dump_manager_android.h"
#endif

namespace base {
class Time;
}

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}  // namespace net

namespace data_reduction_proxy {
class DataReductionProxyData;
struct DataReductionProxyPageLoadTiming;

// Manages pingbacks about page load timing information to the data saver proxy
// server. This class is not thread safe.
class DataReductionProxyPingbackClientImpl
    : public DataReductionProxyPingbackClient,
      public net::URLFetcherDelegate
#if defined(OS_ANDROID)
    ,
      public breakpad::CrashDumpManager::Observer
#endif
{
 public:
  // The caller must ensure that |url_request_context| remains alive for the
  // lifetime of the |DataReductionProxyPingbackClientImpl| instance.
  explicit DataReductionProxyPingbackClientImpl(
      net::URLRequestContextGetter* url_request_context);
  ~DataReductionProxyPingbackClientImpl() override;

 protected:
  // Generates a float in the range [0, 1). Virtualized in testing.
  virtual float GenerateRandomFloat() const;

  // Returns the current time. Virtualized in testing.
  virtual base::Time CurrentTime() const;

  // The amount of time to wait on a crash processing..
  virtual base::TimeDelta GetCrashReportingDelay() const;

 private:
#if defined(OS_ANDROID)
  // CrashDumpManager::Observer:
  void OnCrashDumpProcessed(
      const breakpad::CrashDumpManager::CrashDumpDetails& details) override;
#endif

  // DataReductionProxyPingbackClient:
  void SendPingback(const DataReductionProxyData& data,
                    const DataReductionProxyPageLoadTiming& timing) override;
  void SetPingbackReportingFraction(float pingback_reporting_fraction) override;

  // URLFetcherDelegate implmentation:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Whether a pingback should be sent.
  bool ShouldSendPingback() const;

#if defined(OS_ANDROID)
  // Adds a renderer crash pingback to a map of crashed renderers to data. If it
  // is not removed within 5 seconds, it's removed automatically.
  void AddRequestToCrashMap(const DataReductionProxyData& request_data,
                            const DataReductionProxyPageLoadTiming& timing);

  // Removes a renderer crash reports from the crash map and reports the crash
  // as not an OOM.
  void RemoveFromCrashMap(int process_host_id);
#endif

  // Creates the proto page load report and adds it to the current pending batch
  // of reports. If there is no outstanding request, sends the batched report.
  void CreateReport(const DataReductionProxyData& request_data,
                    const DataReductionProxyPageLoadTiming& timing,
                    PageloadMetrics_RendererCrashType crash_type);

  // Creates an URLFetcher that will POST to |secure_proxy_url_| using
  // |url_request_context_|. The max retries is set to 5.
  // |data_to_send_| will be used to fill the body of the Fetcher, and will be
  // reset to an empty RecordPageloadMetricsRequest.
  void CreateFetcherForDataAndStart();

  net::URLRequestContextGetter* url_request_context_;

  // The URL for the data saver proxy's ping back service.
  const GURL pingback_url_;

  // The currently running fetcher.
  std::unique_ptr<net::URLFetcher> current_fetcher_;

  // Serialized data to send to the data saver proxy server.
  RecordPageloadMetricsRequest metrics_request_;

  // The probability of sending a pingback to the server.
  float pingback_reporting_fraction_;

#if defined(OS_ANDROID)
  // Maps host process ID to information for the pingback. Items are added to
  // the crash map when the renderer process crashes. If OnCrashDumpProcessed is
  // not called within 5 seconds, the report is sent as if no OOM occured.
  std::map<int,
           std::pair<DataReductionProxyData, DataReductionProxyPageLoadTiming>>
      crash_map_;

  ScopedObserver<breakpad::CrashDumpManager,
                 breakpad::CrashDumpManager::Observer>
      scoped_observer_;
#endif

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyPingbackClientImpl);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_DATA_REDUCTION_PROXY_PINGBACK_CLIENT_IMPL_H_
