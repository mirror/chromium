// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_HISTOGRAM_CONTROLLER_H_
#define CONTENT_BROWSER_HISTOGRAM_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "content/common/histogram.mojom.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace content {

class HistogramSubscriber;

// HistogramController is used on the browser process to collect histogram data.
// Only the browser UI thread is allowed to interact with the
// HistogramController object, except for RegisterClientOnIoThread().
class HistogramController {
 public:
  // Returns the HistogramController object for the current process, or NULL if
  // none.
  static HistogramController* GetInstance();

  // Normally instantiated when the child process is launched. Only one instance
  // should be created per process.
  HistogramController();
  virtual ~HistogramController();

  // Register the subscriber so that it will be called when for example
  // OnHistogramDataCollected is returning histogram data from a child process.
  // This is called on UI thread.
  void Register(HistogramSubscriber* subscriber);

  // Unregister the subscriber so that it will not be called when for example
  // OnHistogramDataCollected is returning histogram data from a child process.
  // Safe to call even if caller is not the current subscriber.
  void Unregister(const HistogramSubscriber* subscriber);

  // Contact all registered clients and get their histogram data.
  void GetHistogramData(int sequence_number);

  // Register |client| as a provider of histogram data from a remote process.
  // |client| will be queried on the UI thread for histogram data when
  // GetHistogramData() is called. Must be called on the UI thread.
  void RegisterClient(mojom::HistogramCollectorClientPtr client);

  // Like RegisterClient() above, but registers client on the IO thread.
  // |client| will only be used on the IO thread.
  void RegisterClientOnIoThread(mojom::HistogramCollectorClientPtr client);

 private:
  friend struct base::DefaultSingletonTraits<HistogramController>;

  // Notify the |subscriber_| that it should expect at least |pending_processes|
  // additional calls to OnHistogramDataCollected().  OnPendingProcess() may be
  // called repeatedly; the last call will have |end| set to true, indicating
  // that there is no longer a possibility for the count of pending processes to
  // increase.  This is called on the UI thread.
  void OnPendingProcesses(int sequence_number, int pending_processes, bool end);

  // Send the |histogram| back to the |subscriber_|. This can be called from any
  // thread.
  void OnHistogramDataCollected(
      int sequence_number,
      const std::vector<std::string>& pickled_histograms);

  // Collect histogram data from each of a given set of |clients|. This method
  // can be called on either the IO or UI thread, but the appropriate client set
  // must be used. |sequence_number| and |end| will be passsed to
  // OnPendingProcess() after requests are made to all of the clients.
  void GetHistogramDataFromClients(
      int sequence_number,
      mojo::InterfacePtrSet<mojom::HistogramCollectorClient>* clients,
      bool end);

  mojo::InterfacePtrSet<mojom::HistogramCollectorClient> ui_thread_clients_;
  mojo::InterfacePtrSet<mojom::HistogramCollectorClient> io_thread_clients_;
  HistogramSubscriber* subscriber_;

  DISALLOW_COPY_AND_ASSIGN(HistogramController);
};

}  // namespace content

#endif  // CONTENT_BROWSER_HISTOGRAM_CONTROLLER_H_
