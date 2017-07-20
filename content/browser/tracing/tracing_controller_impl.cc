// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "content/browser/tracing/tracing_controller_impl.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_tracing.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_split.h"
#include "base/trace_event/trace_config.h"
#include "base/values.h"
#include "build/build_config.h"
#include "content/browser/tracing/file_tracing_provider_impl.h"
#include "content/browser/tracing/power_tracing_agent.h"
#include "content/browser/tracing/tracing_ui.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/tracing_controller.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/common/data_pipe_drainer.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "services/resource_coordinator/public/cpp/tracing/chrome_trace_event_agent.h"
#include "services/service_manager/public/cpp/connector.h"

#if (defined(OS_POSIX) && defined(USE_UDEV)) || defined(OS_WIN) || \
    defined(OS_MACOSX)
#define ENABLE_POWER_TRACING
#endif

#if defined(ENABLE_POWER_TRACING)
#include "content/browser/tracing/power_tracing_agent.h"
#endif

#if defined(OS_CHROMEOS)
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/debug_daemon_client.h"
#include "content/browser/tracing/arc_tracing_agent_impl.h"
#include "content/browser/tracing/cros_tracing_agent.h"
#endif

#if defined(OS_WIN)
#include "content/browser/tracing/etw_tracing_agent_win.h"
#endif

namespace content {

namespace {

base::LazyInstance<TracingControllerImpl>::Leaky g_controller =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

TracingController* TracingController::GetInstance() {
  return TracingControllerImpl::GetInstance();
}

TracingControllerImpl::TracingControllerImpl() {
  auto* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();
  connector->BindInterface("content_browser", &coordinator_);

// Register tracing agents.
#if defined(ENABLE_POWER_TRACING)
  agents_.push_back(base::MakeUnique<PowerTracingAgent>(connector));
#endif

#if defined(OS_CHROMEOS)
  agents_.push_back(base::MakeUnique<CrOSTracingAgent>(connector));
  agents_.push_back(base::MakeUnique<ArcTracingAgentImpl>(connector));
#elif defined(OS_WIN)
  agents_.push_back(base::MakeUnique<EtwTracingAgent>(connector));
#endif

  auto chrome_agent =
      base::MakeUnique<tracing::ChromeTraceEventAgent>(connector);
  chrome_agent->AddMetadataGeneratorFunction(
      base::BindRepeating(&TracingControllerImpl::GenerateTracingMetadataDict,
                          base::Unretained(this)));
  agents_.push_back(std::move(chrome_agent));

  // Deliberately leaked, like this class.
  base::FileTracing::SetProvider(new FileTracingProviderImpl);
}

TracingControllerImpl::~TracingControllerImpl() {
  // This is a Leaky instance.
  NOTREACHED();
}

std::unique_ptr<base::DictionaryValue>
TracingControllerImpl::GenerateTracingMetadataDict() const {
  return base::MakeUnique<base::DictionaryValue>();
}

TracingControllerImpl* TracingControllerImpl::GetInstance() {
  return g_controller.Pointer();
}

bool TracingControllerImpl::GetCategories(
    const GetCategoriesDoneCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  coordinator_->GetCategories(base::BindRepeating(
      [](const GetCategoriesDoneCallback& callback, bool success,
         const std::string& categories) {
        const std::vector<std::string> split = base::SplitString(
            categories, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
        std::set<std::string> category_set;
        for (const auto& category : split) {
          category_set.insert(category);
        }
        callback.Run(category_set);
      },
      callback));
  // TODO(chiniforooshan): The actual success value should be sent by the
  // callback asynchronously.
  return true;
}

bool TracingControllerImpl::StartTracing(
    const base::trace_event::TraceConfig& trace_config,
    const StartTracingDoneCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  is_tracing_ = true;
  coordinator_->StartTracing(
      trace_config.ToString(),
      base::BindRepeating(
          [](const StartTracingDoneCallback& callback, bool success) {
            if (!callback.is_null())
              callback.Run();
          },
          callback));
  // TODO(chiniforooshan): The actual success value should be sent by the
  // callback asynchronously.
  return true;
}

bool TracingControllerImpl::StopTracing(
    const scoped_refptr<TraceDataEndpoint>& trace_data_endpoint) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  trace_data_endpoint_ = trace_data_endpoint;
  mojo::DataPipe data_pipe;
  coordinator_->StopAndFlush(
      std::move(data_pipe.producer_handle),
      base::BindRepeating(&TracingControllerImpl::OnMetadataAvailable,
                          base::Unretained(this)));
  drainer_.reset(new mojo::common::DataPipeDrainer(
      this, std::move(data_pipe.consumer_handle)));
  // TODO(chiniforooshan): Is the return value used anywhere?
  return true;
}

bool TracingControllerImpl::GetTraceBufferUsage(
    const GetTraceBufferUsageCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  coordinator_->RequestBufferUsage(base::BindRepeating(
      [](const GetTraceBufferUsageCallback& callback, bool success,
         float percent_full, uint32_t approximate_count) {
        callback.Run(percent_full, approximate_count);
      },
      callback));
  // TODO(chiniforooshan): The actual success value should be sent by the
  // callback asynchronously.
  return true;
}

bool TracingControllerImpl::IsTracing() const {
  return is_tracing_;
}

void TracingControllerImpl::RegisterTracingUI(TracingUI* tracing_ui) {
  DCHECK(tracing_uis_.find(tracing_ui) == tracing_uis_.end());
  tracing_uis_.insert(tracing_ui);
}

void TracingControllerImpl::UnregisterTracingUI(TracingUI* tracing_ui) {
  std::set<TracingUI*>::iterator it = tracing_uis_.find(tracing_ui);
  DCHECK(it != tracing_uis_.end());
  tracing_uis_.erase(it);
}

void TracingControllerImpl::OnDataAvailable(const void* data,
                                            size_t num_bytes) {
  if (trace_data_endpoint_) {
    const std::string chunk(static_cast<const char*>(data), num_bytes);
    trace_data_endpoint_->ReceiveTraceChunk(
        base::MakeUnique<std::string>(chunk));
  }
}

void TracingControllerImpl::OnDataComplete() {}

void TracingControllerImpl::OnMetadataAvailable(
    std::unique_ptr<base::DictionaryValue> metadata) {
  if (trace_data_endpoint_)
    trace_data_endpoint_->ReceiveTraceFinalContents(std::move(metadata));
  trace_data_endpoint_ = nullptr;
  is_tracing_ = false;
}

}  // namespace content
