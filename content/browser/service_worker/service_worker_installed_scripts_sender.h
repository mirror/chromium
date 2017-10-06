// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INSTALLED_SCRIPTS_SENDER_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INSTALLED_SCRIPTS_SENDER_H_

#include <queue>

#include "content/common/service_worker/service_worker_installed_scripts_manager.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

struct HttpResponseInfoIOBuffer;
class ServiceWorkerVersion;

// ServiceWorkerInstalledScriptsSender serves the service worker's installed
// scripts from ServiceWorkerStorage to the renderer through Mojo data pipes.
// ServiceWorkerInstalledScriptsSender is owned by ServiceWorkerVersion. It is
// created for worker startup and lives as long as the worker is running.
//
// SWInstalledScriptsSender has two states: sending scripts and idle. In the
// sending state, the sender sends the installed scripts queued in
// |pending_scripts_| to the renderer. |pending_scripts_| are initially set to
// all of installed scripts. Once all of queued scripts are sent, the sender
// goes to the idle state. When RequestInstalledScript() is called over Mojo
// IPC, the request is queued to |pending_scripts_| and the sender gets back to
// the sending state if it's idle.
class CONTENT_EXPORT ServiceWorkerInstalledScriptsSender
    : public mojom::ServiceWorkerInstalledScriptsManagerHost {
 public:
  // Do not change the order. This is used for UMA.
  enum class FinishedReason {
    kNotFinished = 0,
    kSuccess = 1,
    kNoHttpInfoError = 2,
    kCreateDataPipeError = 3,
    kConnectionError = 4,
    kResponseReaderError = 5,
    kMetaDataSenderError = 6,
    // Add a new type here, then update kMaxValue and enums.xml.
    kMaxValue = kMetaDataSenderError,
  };

  // |owner| must be an installed service worker.
  explicit ServiceWorkerInstalledScriptsSender(ServiceWorkerVersion* owner);

  ~ServiceWorkerInstalledScriptsSender() override;

  // Creates a Mojo struct (mojom::ServiceWorkerInstalledScriptsInfo) and sets
  // it with the information to create WebServiceWorkerInstalledScriptsManager
  // on the renderer.
  mojom::ServiceWorkerInstalledScriptsInfoPtr CreateInfoAndBind();

  // Starts sending installed scripts to the worker.
  void Start();

  // |finished_reason_| is updated when the sender goes to the idle state. Once
  // the worker has been successfully launched, the finished reason should be
  // kSuccess, or an error code during the latest streaming which aborts
  // starting the worker.
  FinishedReason finished_reason() const { return finished_reason_; }

 private:
  class Sender;

  enum class State {
    kNotStarted,
    kSendingScripts,
    kIdle,
  };

  void StartSendingScript(int64_t resource_id, const GURL& script_url);

  // Stops all tasks even if pending scripts exist and disconnects the pipe to
  // the renderer. Also, if the installed script stored in the disk cache is
  // corrupted, |owner_| is deleted.
  void Abort(FinishedReason reason);

  // Called from |running_sender_|.
  void SendScriptInfoToRenderer(
      std::string encoding,
      std::unordered_map<std::string, std::string> headers,
      mojo::ScopedDataPipeConsumerHandle body_handle,
      uint64_t body_size,
      mojo::ScopedDataPipeConsumerHandle meta_data_handle,
      uint64_t meta_data_size);
  void OnHttpInfoRead(scoped_refptr<HttpResponseInfoIOBuffer> http_info);
  void OnFinishSendingScript(FinishedReason reason);

  void Finish(FinishedReason reason);

  // Implements mojom::ServiceWorkerInstalledScriptsManagerHost.
  void RequestInstalledScript(const GURL& script_url) override;

  bool IsSendingMainScript() const;

  ServiceWorkerVersion* owner_;
  const GURL main_script_url_;
  const int64_t main_script_id_;
  bool has_main_script_streamed_;

  mojo::Binding<mojom::ServiceWorkerInstalledScriptsManagerHost> binding_;
  mojom::ServiceWorkerInstalledScriptsManagerPtr manager_;
  std::unique_ptr<Sender> running_sender_;

  State state_;
  FinishedReason finished_reason_;

  GURL current_sending_url_;
  std::queue<std::pair<int64_t /* resource_id */, GURL>> pending_scripts_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerInstalledScriptsSender);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INSTALLED_SCRIPTS_SENDER_H_
