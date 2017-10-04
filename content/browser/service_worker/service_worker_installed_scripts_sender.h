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
// SWInstalledScriptsSender has three phases: streaming scripts, waiting and
// sending requested scripts. In the streaming state, the sender sends the
// installed scripts to the renderer without any requests. After the streaming
// phase, the sender moves to the waiting phase. In the waiting phase, sender
// waits for RequestInstalledScript. Once RequestInstalledScript comes, the
// sender moves to the sending requested scripts phase until finishing to send
// all requested scripts. If RequestInstalledScript is called during the
// streaming phase, the request will be queued and processed after the streaming
// phase.
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

  // Returns true if streaming the installed scripts has been finished. Even
  // though IsStreamingFinished() returns ture, you can call
  // RequestInstalledScript for getting the installed script.
  bool IsStreamingFinished() const;
  FinishedReason finished_reason() const { return finished_reason_; }

 private:
  class Sender;

  enum class State {
    kNotStarted,
    kSendingMainScript,
    kSendingImportedScript,
    kWaiting,
    kSendingRequestedScript,
  };

  void StartSendingScript(int64_t resource_id);

  // Called from |running_sender_|.
  void SendScriptInfoToRenderer(
      std::string encoding,
      std::unordered_map<std::string, std::string> headers,
      mojo::ScopedDataPipeConsumerHandle body_handle,
      uint64_t body_size,
      mojo::ScopedDataPipeConsumerHandle meta_data_handle,
      uint64_t meta_data_size);
  void OnHttpInfoRead(scoped_refptr<HttpResponseInfoIOBuffer> http_info);
  void OnFinishSendingScript();
  void OnAbortSendingScript(FinishedReason reason);

  const GURL& CurrentSendingURL();

  void UpdateState(State state);
  void Finish(FinishedReason reason);

  // Implements mojom::ServiceWorkerInstalledScriptsManagerHost.
  void RequestInstalledScript(const GURL& script_url) override;

  ServiceWorkerVersion* owner_;
  const GURL main_script_url_;
  const int main_script_id_;

  mojo::Binding<mojom::ServiceWorkerInstalledScriptsManagerHost> binding_;
  mojom::ServiceWorkerInstalledScriptsManagerPtr manager_;
  std::unique_ptr<Sender> running_sender_;
  State state_;
  FinishedReason finished_reason_;
  std::map<int64_t /* resource_id */, GURL> imported_scripts_;
  std::map<int64_t /* resource_id */, GURL>::iterator imported_script_iter_;

  // Scripts sent after streaming all of installed scripts requested by
  // RequestInstalledScript().
  std::queue<std::pair<int64_t /* resource_id */, GURL>> requested_scripts_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerInstalledScriptsSender);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_INSTALLED_SCRIPTS_SENDER_H_
