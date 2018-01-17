// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/net/reporting_service_proxy.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/values.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/reporting/reporting_report.h"
#include "net/reporting/reporting_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "third_party/WebKit/public/platform/reporting.mojom.h"
#include "url/gurl.h"

namespace content {

namespace {

class ReportingServiceProxyImpl : public blink::mojom::ReportingServiceProxy {
 public:
  ReportingServiceProxyImpl(
      scoped_refptr<net::URLRequestContextGetter> request_context_getter)
      : request_context_getter_(std::move(request_context_getter)) {}

  // blink::mojom::ReportingServiceProxy:

  void QueueInterventionReport(const GURL& url,
                               const std::string& message,
                               const std::string& source_file,
                               int line_number,
                               int column_number) override {
    auto body = std::make_unique<base::DictionaryValue>();
    body->SetKey("message", base::Value(message));
    body->SetKey("sourceFile", base::Value(source_file));
    body->SetKey("lineNumber", base::Value(line_number));
    body->SetKey("columnNumber", base::Value(column_number));
    QueueReport(url, "default", "intervention", std::move(body));
  }

  void QueueDeprecationReport(const GURL& url,
                              const std::string& id,
                              base::Time anticipatedRemoval,
                              const std::string& message,
                              const std::string& source_file,
                              int line_number,
                              int column_number) override {
    auto body = std::make_unique<base::DictionaryValue>();
    body->SetKey("id", base::Value(id));
    if (anticipatedRemoval.is_null())
      body->SetKey("anticipatedRemoval",
                   base::Value(anticipatedRemoval.ToDoubleT()));
    body->SetKey("message", base::Value(message));
    body->SetKey("sourceFile", base::Value(source_file));
    body->SetKey("lineNumber", base::Value(line_number));
    body->SetKey("columnNumber", base::Value(column_number));
    QueueReport(url, "default", "deprecation", std::move(body));
  }

  void QueueCspViolationReport(const GURL& url,
                               const std::string& group,
                               const std::string& document_uri,
                               const std::string& referrer,
                               const std::string& violated_directive,
                               const std::string& effective_directive,
                               const std::string& original_policy,
                               const std::string& disposition,
                               const std::string& blocked_uri,
                               int line_number,
                               int column_number,
                               const std::string& source_file,
                               int status_code,
                               const std::string& script_sample) override {
    auto body = std::make_unique<base::DictionaryValue>();
    body->SetKey("document-uri", base::Value(document_uri));
    body->SetKey("referrer", base::Value(referrer));
    body->SetKey("violated-directive", base::Value(violated_directive));
    body->SetKey("effective-directive", base::Value(effective_directive));
    body->SetKey("original-policy", base::Value(original_policy));
    body->SetKey("disposition", base::Value(disposition));
    body->SetKey("blocked-uri", base::Value(blocked_uri));
    if (line_number)
      body->SetKey("line-number", base::Value(line_number));
    if (column_number)
      body->SetKey("column-number", base::Value(column_number));
    body->SetKey("source-file", base::Value(source_file));
    if (status_code)
      body->SetKey("status-code", base::Value(status_code));
    body->SetKey("script-sample", base::Value(script_sample));
    QueueReport(url, group, "csp", std::move(body));
  }

 private:
  void QueueReport(const GURL& url,
                   const std::string& group,
                   const std::string& type,
                   std::unique_ptr<base::Value> body) {
    net::URLRequestContext* request_context =
        request_context_getter_->GetURLRequestContext();
    if (!request_context) {
      net::ReportingReport::RecordReportDiscardedForNoURLRequestContext();
      return;
    }

    net::ReportingService* reporting_service =
        request_context->reporting_service();
    if (!reporting_service) {
      net::ReportingReport::RecordReportDiscardedForNoReportingService();
      return;
    }

    reporting_service->QueueReport(url, group, type, std::move(body));
  }

  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
};

void CreateReportingServiceProxyOnNetworkTaskRunner(
    blink::mojom::ReportingServiceProxyRequest request,
    scoped_refptr<net::URLRequestContextGetter> request_context_getter) {
  mojo::MakeStrongBinding(std::make_unique<ReportingServiceProxyImpl>(
                              std::move(request_context_getter)),
                          std::move(request));
}

}  // namespace

// static
void CreateReportingServiceProxy(
    StoragePartition* storage_partition,
    blink::mojom::ReportingServiceProxyRequest request) {
  scoped_refptr<net::URLRequestContextGetter> request_context_getter(
      storage_partition->GetURLRequestContext());
  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner(
      request_context_getter->GetNetworkTaskRunner());
  network_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&CreateReportingServiceProxyOnNetworkTaskRunner,
                     std::move(request), std::move(request_context_getter)));
}

}  // namespace content
