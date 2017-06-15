// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/histogram_controller.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/process/process_handle.h"
#include "content/browser/histogram_subscriber.h"
#include "content/common/child_process_messages.h"
#include "content/public/browser/browser_child_process_host_iterator.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/process_type.h"

namespace content {

HistogramController* HistogramController::GetInstance() {
  return base::Singleton<HistogramController>::get();
}

HistogramController::HistogramController()
    : ui_thread_clients_(base::MakeUnique<ClientPtrSet>()),
      io_thread_clients_(base::MakeUnique<ClientPtrSet>()),
      subscriber_(nullptr) {}

HistogramController::~HistogramController() {
  // |io_thread_clients_| must be destroyed on the IO thread.
  BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE,
                            std::move(io_thread_clients_));
}

void HistogramController::Register(HistogramSubscriber* subscriber) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!subscriber_);
  subscriber_ = subscriber;
}

void HistogramController::Unregister(const HistogramSubscriber* subscriber) {
  DCHECK_EQ(subscriber_, subscriber);
  subscriber_ = NULL;
}

void HistogramController::GetHistogramData(int sequence_number) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Collect the histogram data from clients on the UI thread.
  GetHistogramDataFromClients(sequence_number, ui_thread_clients_.get(),
                              false /* end */);

  // Collect the histogram data from clients on the IO thread.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&HistogramController::GetHistogramDataFromClients,
                     base::Unretained(this), sequence_number,
                     io_thread_clients_.get(), true /* end */));
}

void HistogramController::RegisterClient(
    mojom::HistogramCollectorClientPtr client) {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    ui_thread_clients_->AddPtr(std::move(client));
  } else if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    io_thread_clients_->AddPtr(std::move(client));
  } else {
    NOTREACHED() << "Client will not be added, this thread not supported!";
  }
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

void HistogramController::GetHistogramDataFromClients(
    int sequence_number,
    mojo::InterfacePtrSet<mojom::HistogramCollectorClient>* clients,
    bool end) {
  DCHECK(clients);
  int pending_processes = 0;
  clients->ForAllPtrs([this, sequence_number, &pending_processes](
                          mojom::HistogramCollectorClient* client) {
    client->RequestNonPersistentHistogramData(
        base::BindOnce(&HistogramController::OnHistogramDataCollected,
                       base::Unretained(this), sequence_number));
    pending_processes += 1;
  });

  // If we're on the UI thread, notify the subscriber synchronously...
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    OnPendingProcesses(sequence_number, pending_processes, end);
    return;
  }

  // ...otherwise, notify the subscriber asynchronously on the UI thread.
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&HistogramController::OnPendingProcesses,
                                     base::Unretained(this), sequence_number,
                                     pending_processes, end));
}

}  // namespace content
