// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/histogram_controller.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/process/process_handle.h"
#include "content/browser/histogram_subscriber.h"
#include "content/common/child_histogram.mojom.h"
#include "content/common/child_process_messages.h"
#include "content/public/browser/browser_child_process_host_iterator.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/bind_interface_helpers.h"
#include "content/public/common/child_process_host.h"
#include "content/public/common/process_type.h"

namespace content {

HistogramController* HistogramController::GetInstance() {
  return base::Singleton<HistogramController, base::LeakySingletonTraits<
                                                  HistogramController>>::get();
}

HistogramController::HistogramController() : subscriber_(NULL) {
}

HistogramController::~HistogramController() {
}

void HistogramController::OnPendingProcesses(int sequence_number,
                                             int pending_processes,
                                             bool end) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (subscriber_)
    subscriber_->OnPendingProcesses(sequence_number, pending_processes, end);
}

void HistogramController::OnHistogramDataCollected(
    int sequence_number,
    const std::vector<std::string>& pickled_histograms) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&HistogramController::OnHistogramDataCollected,
                       base::Unretained(this), sequence_number,
                       pickled_histograms));
    return;
  }
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (subscriber_) {
    subscriber_->OnHistogramDataCollected(sequence_number,
                                          pickled_histograms);
  }
}

void HistogramController::Register(HistogramSubscriber* subscriber) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!subscriber_);
  subscriber_ = subscriber;
}

void HistogramController::Unregister(
    const HistogramSubscriber* subscriber) {
  DCHECK_EQ(subscriber_, subscriber);
  subscriber_ = NULL;
}

// static
template <class T>
content::mojom::ChildHistogram& HistogramController::GetChildHistogramInterface(
    ChildHistogramMap<T>* child_histogram_map,
    T* host) {
  // First check the map.
  auto it = child_histogram_map->find(host);
  if (it != child_histogram_map->end()) {
    return *it->second;
  }
  // Not found, so bind a new interface.
  content::mojom::ChildHistogramPtr child_histogram;
  content::BindInterface(host, &child_histogram);
  // Broken pipe means remove this from the map. The map size is a proxy for
  // the number of known processes
  child_histogram.set_connection_error_handler(
      base::Bind(&HistogramController::RemoveChildHistogramInterface<T>,
                 base::Unretained(child_histogram_map), host));
  auto result = child_histogram_map->emplace(host, std::move(child_histogram));
  return *(result.first->second);
}

// static
template <class T>
void HistogramController::RemoveChildHistogramInterface(
    ChildHistogramMap<T>* child_histogram_map,
    T* host) {
  child_histogram_map->erase(host);
}

void HistogramController::GetHistogramDataFromChildProcesses(
    int sequence_number) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  for (BrowserChildProcessHostIterator iter; !iter.Done(); ++iter) {
    const ChildProcessData& data = iter.GetData();

    // Only get histograms from content process types; skip "embedder" process
    // types.
    if (data.process_type >= PROCESS_TYPE_CONTENT_END)
      continue;

    // In some cases, there may be no child process of the given type (for
    // example, the GPU process may not exist and there may instead just be a
    // GPU thread in the browser process). If that's the case, then the process
    // handle will be base::kNullProcessHandle and we shouldn't ask it for data.
    if (data.handle == base::kNullProcessHandle)
      continue;

    GetChildHistogramInterface(&child_histogram_interfaces_, iter.GetHost())
        .GetChildNonPersistentHistogramData(
            base::Bind(&HistogramController::OnHistogramDataCollected,
                       base::Unretained(this), sequence_number));
  }

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&HistogramController::OnPendingProcesses,
                     base::Unretained(this), sequence_number,
                     child_histogram_interfaces_.size(), true));
}

void HistogramController::GetHistogramData(int sequence_number) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  for (RenderProcessHost::iterator it(RenderProcessHost::AllHostsIterator());
       !it.IsAtEnd(); it.Advance()) {
    GetChildHistogramInterface(&renderer_histogram_interfaces_,
                               it.GetCurrentValue())
        .GetChildNonPersistentHistogramData(
            base::Bind(&HistogramController::OnHistogramDataCollected,
                       base::Unretained(this), sequence_number));
  }
  OnPendingProcesses(sequence_number, renderer_histogram_interfaces_.size(),
                     false);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&HistogramController::GetHistogramDataFromChildProcesses,
                     base::Unretained(this), sequence_number));
}

}  // namespace content
