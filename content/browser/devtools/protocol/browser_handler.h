// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_PROTOCOL_BROWSER_HANDLER_H_
#define CONTENT_BROWSER_DEVTOOLS_PROTOCOL_BROWSER_HANDLER_H_

#include <unordered_map>

#include "base/macros.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/histogram_samples.h"
#include "base/strings/string_piece.h"
#include "content/browser/devtools/protocol/browser.h"
#include "content/browser/devtools/protocol/devtools_domain_handler.h"

namespace content {
namespace protocol {

class BrowserHandler : public DevToolsDomainHandler, public Browser::Backend {
 public:
  BrowserHandler();
  ~BrowserHandler() override;

  void Wire(UberDispatcher* dispatcher) override;

  // Protocol methods.
  Response GetVersion(std::string* protocol_version,
                      std::string* product,
                      std::string* revision,
                      std::string* user_agent,
                      std::string* js_version) override;

  Response GetHistograms(
      Maybe<std::string> in_query,
      Maybe<bool> in_deltaFromLastSnapshot,
      std::unique_ptr<Array<Browser::Histogram>>* histograms) override;

  Response GetHistogram(
      const std::string& in_name,
      Maybe<bool> in_deltaFromLastSnapshot,
      std::unique_ptr<Browser::Histogram>* out_histogram) override;

  Response TakeHistogramSnapshot() override;

 private:
  std::unique_ptr<base::HistogramSamples> GetHistogramSnapshotSamples(
      const base::HistogramBase& in_histogram,
      const bool in_deltaFromLastSnapshot);

  typedef std::unordered_map<base::StringPiece,
                             std::shared_ptr<base::HistogramSamples>,
                             base::StringPieceHash>
      HistogramMap;
  std::unique_ptr<HistogramMap> histogram_snapshot_;

  DISALLOW_COPY_AND_ASSIGN(BrowserHandler);
};

}  // namespace protocol
}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_PROTOCOL_BROWSER_HANDLER_H_
