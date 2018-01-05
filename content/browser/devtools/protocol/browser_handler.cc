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

struct OrderByName {
  bool operator()(const base::HistogramBase& a,
                  const base::HistogramBase& b) const {
    return strcmp(a.histogram_name(), b.histogram_name()) < 0;
  }

  bool operator()(const base::HistogramBase* const a,
                  const base::HistogramBase* const b) const {
    return (*this)(*a, *b);
  }
};

}  // namespace

Response BrowserHandler::GetHistograms(
    std::unique_ptr<Browser::Histograms>* const out_histograms) {
  *out_histograms = std::make_unique<Browser::Histograms>();
  base::StatisticsRecorder::Histograms in_histograms;
  base::StatisticsRecorder::GetHistograms(&in_histograms);
  std::sort(in_histograms.begin(), in_histograms.end(), OrderByName());

  for (const base::HistogramBase* const in_histogram : in_histograms) {
    const std::unique_ptr<const base::HistogramSamples> in_buckets =
        in_histogram->SnapshotSamples();
    DCHECK(in_buckets);

    std::unique_ptr<Browser::Buckets> out_buckets =
        std::make_unique<Browser::Buckets>();

    for (const std::unique_ptr<base::SampleCountIterator> bucket_it =
             in_buckets->Iterator();
         !bucket_it->Done(); bucket_it->Next()) {
      base::HistogramBase::Count count;
      base::HistogramBase::Sample low;
      int64_t high;
      bucket_it->Get(&low, &high, &count);
      out_buckets->addItem(Browser::Bucket::Create()
                               .SetLow(low)
                               .SetHigh(high)
                               .SetCount(count)
                               .Build());
    }

    (*out_histograms)
        ->addItem(Browser::Histogram::Create()
                      .SetName(in_histogram->histogram_name())
                      .SetSum(in_buckets->sum())
                      .SetCount(in_buckets->TotalCount())
                      .SetBuckets(std::move(out_buckets))
                      .Build());
  }
  return Response::OK();
}

}  // namespace protocol
}  // namespace content
