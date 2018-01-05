// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/protocol/browser_handler.h"

#include <string.h>
#include <algorithm>

#include "base/metrics/histogram_base.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/statistics_recorder.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/common/content_client.h"
#include "content/public/common/user_agent.h"
#include "v8/include/v8-version-string.h"

namespace content {
namespace protocol {

BrowserHandler::BrowserHandler()
    : DevToolsDomainHandler(Browser::Metainfo::domainName) {}

BrowserHandler::~BrowserHandler() {}

void BrowserHandler::Wire(UberDispatcher* dispatcher) {
  Browser::Dispatcher::wire(dispatcher, this);
}

Response BrowserHandler::GetVersion(std::string* protocol_version,
                                    std::string* product,
                                    std::string* revision,
                                    std::string* user_agent,
                                    std::string* js_version) {
  *protocol_version = DevToolsAgentHost::GetProtocolVersion();
  *revision = GetWebKitRevision();
  *product = GetContentClient()->GetProduct();
  *user_agent = GetContentClient()->GetUserAgent();
  *js_version = V8_VERSION_STRING;
  return Response::OK();
}

namespace Browser {

using Histograms = Array<Histogram>;
using Buckets = Array<Bucket>;

}  // namespace Browser

namespace {

using base::HistogramBase;
using base::StatisticsRecorder;
using std::make_unique;
using std::unique_ptr;

// Orders histograms by name.
struct OrderByName {
  bool operator()(const HistogramBase& a, const HistogramBase& b) const {
    return strcmp(a.histogram_name(), b.histogram_name()) < 0;
  }

  bool operator()(const HistogramBase* const a,
                  const HistogramBase* const b) const {
    return (*this)(*a, *b);
  }
};

// Converts an histogram.
unique_ptr<Browser::Histogram> Convert(const HistogramBase& in_histogram) {
  const unique_ptr<const base::HistogramSamples> in_buckets =
      in_histogram.SnapshotSamples();
  DCHECK(in_buckets);

  unique_ptr<Browser::Buckets> out_buckets = make_unique<Browser::Buckets>();

  for (const unique_ptr<base::SampleCountIterator> bucket_it =
           in_buckets->Iterator();
       !bucket_it->Done(); bucket_it->Next()) {
    HistogramBase::Count count;
    HistogramBase::Sample low;
    int64_t high;
    bucket_it->Get(&low, &high, &count);
    out_buckets->addItem(Browser::Bucket::Create()
                             .SetLow(low)
                             .SetHigh(high)
                             .SetCount(count)
                             .Build());
  }

  return Browser::Histogram::Create()
      .SetName(in_histogram.histogram_name())
      .SetSum(in_buckets->sum())
      .SetCount(in_buckets->TotalCount())
      .SetBuckets(std::move(out_buckets))
      .Build();
}

}  // namespace

Response BrowserHandler::GetHistograms(
    unique_ptr<Browser::Histograms>* const out_histograms) {
  // Get all histograms.
  StatisticsRecorder::Histograms in_histograms;
  StatisticsRecorder::GetHistograms(&in_histograms);

  // Sort histograms by name.
  std::sort(in_histograms.begin(), in_histograms.end(), OrderByName());

  // Convert histograms.
  DCHECK(out_histograms);
  *out_histograms = make_unique<Browser::Histograms>();
  for (const HistogramBase* const in_histogram : in_histograms) {
    DCHECK(in_histogram);
    (*out_histograms)->addItem(Convert(*in_histogram));
  }

  return Response::OK();
}

Response BrowserHandler::GetHistogram(
    const std::string& in_name,
    unique_ptr<Browser::Histogram>* const out_histogram) {
  // Get histogram by name.
  const HistogramBase* const in_histogram =
      StatisticsRecorder::FindHistogram(in_name);
  if (!in_histogram)
    return Response::InvalidParams("Cannot find histogram: " + in_name);

  // Convert histogram.
  DCHECK(out_histogram);
  *out_histogram = Convert(*in_histogram);

  return Response::OK();
}

}  // namespace protocol
}  // namespace content
