// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TRACING_BACKGROUND_TRACING_MANAGER_IMPL_H_
#define CONTENT_BROWSER_TRACING_BACKGROUND_TRACING_MANAGER_IMPL_H_

#include "base/lazy_instance.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/tracing/tracing_controller_impl.h"
#include "content/public/browser/background_tracing_config.h"
#include "content/public/browser/background_tracing_manager.h"

namespace content {

class BackgroundTracingManagerImpl : public content::BackgroundTracingManager {
 public:
  static BackgroundTracingManagerImpl* GetInstance();

  bool SetActiveScenario(scoped_ptr<BackgroundTracingConfig>,
                         const ReceiveCallback&,
                         bool) override;
  void WhenIdle(IdleCallback idle_callback) override;

  void TriggerNamedEvent(TriggerHandle, StartedFinalizingCallback) override;
  TriggerHandle RegisterTriggerType(const char* trigger_name) override;
  void GetTriggerNameList(std::vector<std::string>* trigger_names) override;

  void InvalidateTriggerHandlesForTesting() override;

 private:
  BackgroundTracingManagerImpl();
  ~BackgroundTracingManagerImpl() override;

  void EnableRecording(base::trace_event::CategoryFilter);
  void EnableRecordingIfConfigNeedsIt();
  void OnFinalizeStarted(scoped_refptr<base::RefCountedString>);
  void OnFinalizeComplete();
  void BeginFinalizing(StartedFinalizingCallback);

  std::string GetTriggerNameFromHandle(TriggerHandle handle) const;
  bool IsTriggerHandleValid(TriggerHandle handle) const;

  bool IsAbleToTriggerTracing(TriggerHandle handle) const;
  bool IsSupportedConfig(BackgroundTracingConfig* config);

  base::trace_event::CategoryFilter GetCategoryFilterForCategoryPreset(
      BackgroundTracingConfig::CategoryPreset) const;

  class TraceDataEndpointWrapper
      : public content::TracingController::TraceDataEndpoint {
   public:
    TraceDataEndpointWrapper(base::Callback<
        void(scoped_refptr<base::RefCountedString>)> done_callback);

    void ReceiveTraceFinalContents(const std::string& file_contents) override;

   private:
    ~TraceDataEndpointWrapper() override;

    base::Callback<void(scoped_refptr<base::RefCountedString>)> done_callback_;
  };

  scoped_ptr<content::BackgroundTracingConfig> config_;
  scoped_refptr<TraceDataEndpointWrapper> data_endpoint_wrapper_;
  std::map<TriggerHandle, std::string> trigger_handles_;
  ReceiveCallback receive_callback_;

  bool is_gathering_;
  bool is_tracing_;
  bool requires_anonymized_data_;
  int trigger_handle_ids_;

  IdleCallback idle_callback_;

  friend struct base::DefaultLazyInstanceTraits<BackgroundTracingManagerImpl>;

  DISALLOW_COPY_AND_ASSIGN(BackgroundTracingManagerImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_TRACING_BACKGROUND_TRACING_MANAGER_IMPL_H_
