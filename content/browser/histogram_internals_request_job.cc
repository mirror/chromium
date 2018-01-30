// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/histogram_internals_request_job.h"

#include "base/metrics/histogram_macros.h"
#include "base/metrics/statistics_recorder.h"
#include "content/browser/histogram_synchronizer.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/escape.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace content {

HistogramInternalsRequestJob::HistogramInternalsRequestJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate)
    : net::URLRequestSimpleJob(request, network_delegate),
      url_(request->url()),
      weak_factory_(this) {}

HistogramInternalsRequestJob::~HistogramInternalsRequestJob() {}

std::string HistogramInternalsRequestJob::GenerateHTML(const GURL& url) {
  const std::string& spec = url.possibly_invalid_spec();
  const url::Parsed& parsed = url.parsed_for_possibly_invalid_spec();
  // + 1 to skip the slash at the beginning of the path.
  int offset = parsed.CountCharactersBefore(url::Parsed::PATH, false) + 1;

  std::string path;
  if (offset < static_cast<int>(spec.size()))
    path = spec.substr(offset);

  HistogramSynchronizer::FetchHistograms();

  std::string unescaped_query;
  std::string unescaped_title("About Histograms");
  if (!path.empty()) {
    unescaped_query = net::UnescapeURLComponent(path,
                                                net::UnescapeRule::NORMAL);
    unescaped_title += " - " + unescaped_query;
  }

  std::string data;
  data.append("<!DOCTYPE html>\n<html>\n<head>\n");
  data.append(
      "<meta http-equiv=\"Content-Security-Policy\" "
      "content=\"object-src 'none'\">");
  data.append("<title>");
  data.append(net::EscapeForHTML(unescaped_title));
  data.append("</title>\n");
  data.append("</head><body>");

  // Display any stats for which we sent off requests the last time.
  data.append("<p>Stats accumulated from browser startup to previous ");
  data.append("page load; reload to get stats as of this page load.</p>\n");

  // A simple interactive form that allows the user to filter by histogram name.
  // The main purpose of this form is to make this feature discoverable.
  data.append(
      "<form><p>Filter (substring of histogram name): <input id=\"filter\" /> "
      "<input type=\"submit\" onclick=\"return applyFilter()\" "
      "value=\"Submit\" /></p></form>\n");

  // TODO(mgiuca): This has escaping issues. DO NOT SUBMIT.
  data.append("<script type=\"text/javascript\">\n");
  data.append("  document.querySelector('#filter').value = '");
  data.append(unescaped_query);
  data.append("';\n");
  data.append("  function applyFilter(e) {\n");
  data.append("    document.location = encodeURIComponent(\n");
  data.append("        document.querySelector('#filter').value);\n");
  data.append("    return false;\n");
  data.append("  }\n");
  data.append("</script>\n");

  data.append("<table width=\"100%\">\n");

  base::StatisticsRecorder::WriteHTMLGraph(unescaped_query, &data);
  return data;
}

void HistogramInternalsRequestJob::Start() {
  // First import histograms from all providers and then start the URL fetch
  // job. It's not possible to call URLRequestSimpleJob::Start through Bind,
  // it ends up re-calling this method, so a small helper method is used.
  content::BrowserThread::PostTaskAndReply(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&base::StatisticsRecorder::ImportProvidedHistograms),
      base::BindOnce(&HistogramInternalsRequestJob::StartUrlRequest,
                     weak_factory_.GetWeakPtr()));
}

int HistogramInternalsRequestJob::GetData(
    std::string* mime_type,
    std::string* charset,
    std::string* data,
    const net::CompletionCallback& callback) const {
  mime_type->assign("text/html");
  charset->assign("UTF8");

  *data = GenerateHTML(url_);
  return net::OK;
}

void HistogramInternalsRequestJob::StartUrlRequest() {
  URLRequestSimpleJob::Start();
}

}  // namespace content
