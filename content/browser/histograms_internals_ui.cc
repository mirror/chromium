// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/histograms_internals_ui.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/i18n/time_formatting.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram.h"
#include "base/metrics/statistics_recorder.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringize_macros.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "base/values.h"
#include "build/build_config.h"
#include "content/browser/histogram_synchronizer.h"
#include "content/grit/content_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/gpu_data_manager_observer.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"

namespace content {
namespace {

WebUIDataSource* CreateHistogramsHTMLSource() {
  WebUIDataSource* source = WebUIDataSource::Create(kChromeUIHistogramHost);

  source->SetJsonPath("strings.js");
  source->AddResourcePath("histograms_internals.js",
                          IDR_HISTOGRAMS_INTERNALS_JS);
  source->SetDefaultResource(IDR_HISTOGRAMS_INTERNALS_HTML);
  source->UseGzip();
  return source;
}

void HistogramsListToValue(base::StatisticsRecorder::Histograms histograms,
                           base::ListValue* out_value) {
  for (base::HistogramBase* histogram : histograms) {
    std::unique_ptr<base::DictionaryValue> histogram_value =
        std::make_unique<base::DictionaryValue>(
            histogram->GetDictionaryValue());
    out_value->Append(std::move(histogram_value));
  }
}

// This class receives javascript messages from the renderer.
// Note that the WebUI infrastructure runs on the UI thread, therefore all of
// this class's methods are expected to run on the UI thread.
class HistogramsMessageHandler
    : public WebUIMessageHandler,
      public base::SupportsWeakPtr<HistogramsMessageHandler> {
 public:
  HistogramsMessageHandler();
  ~HistogramsMessageHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void HandleRequestHistograms(const base::ListValue* args);

  DISALLOW_COPY_AND_ASSIGN(HistogramsMessageHandler);
};

////////////////////////////////////////////////////////////////////////////////
//
// HistogramsMessageHandler
//
////////////////////////////////////////////////////////////////////////////////

HistogramsMessageHandler::HistogramsMessageHandler() {}

HistogramsMessageHandler::~HistogramsMessageHandler() {}

void HistogramsMessageHandler::HandleRequestHistograms(
    const base::ListValue* args) {
  HistogramSynchronizer::FetchHistograms();
  LOG(ERROR) << "HandleRequestHistograms";

  base::ListValue histograms_list;
  HistogramsListToValue(base::StatisticsRecorder::GetSnapshot("Autofill"),
                        &histograms_list);

  std::vector<const base::Value*> out_args;
  out_args.push_back(&histograms_list);
  web_ui()->CallJavascriptFunctionUnsafe("onHistogramsReceived", out_args);
}

void HistogramsMessageHandler::RegisterMessages() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  web_ui()->RegisterMessageCallback(
      "requestHistograms",
      base::Bind(&HistogramsMessageHandler::HandleRequestHistograms,
                 base::Unretained(this)));
}

}  // namespace

HistogramsInternalsUI::HistogramsInternalsUI(WebUI* web_ui)
    : WebUIController(web_ui) {
  web_ui->AddMessageHandler(std::make_unique<HistogramsMessageHandler>());

  // Set up the chrome://histograms/ source.
  BrowserContext* browser_context =
      web_ui->GetWebContents()->GetBrowserContext();
  WebUIDataSource::Add(browser_context, CreateHistogramsHTMLSource());
}

}  // namespace content
