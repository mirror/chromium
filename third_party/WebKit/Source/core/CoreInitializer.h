/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CoreInitializer_h
#define CoreInitializer_h

#include "core/CoreExport.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class HTMLLinkElement;
class HTMLMediaElement;
class InspectedFrames;
class InspectorDOMAgent;
class InspectorSession;
class LinkResource;
class LocalFrame;
class MediaControls;
class Page;
class ShadowRoot;
class WorkerClients;

class CORE_EXPORT CoreInitializer {
  USING_FAST_MALLOC(CoreInitializer);
  WTF_MAKE_NONCOPYABLE(CoreInitializer);

 public:
  static CoreInitializer& GetInstance() {
    DCHECK(instance_);
    return *instance_;
  }

  virtual ~CoreInitializer() {}

  // Should be called by clients before trying to create Frames.
  virtual void Initialize();

  // Callbacks stored in CoreInitializer and set by ModulesInitializer to bypass
  // the inverted dependency from core/ to modules/.
  void CallModulesLocalFrameInit(LocalFrame& frame) const {
    local_frame_init_callback_(frame);
  }
  void CallModulesInstallSupplements(LocalFrame& frame) const {
    chrome_client_install_supplements_callback_(frame);
  }

  void CallModulesProvideLocalFileSystem(WorkerClients& worker_clients) const {
    worker_clients_local_file_system_callback_(worker_clients);
  }

  void CallModulesProvideIndexedDB(WorkerClients& worker_clients) const {
    worker_clients_indexed_db_callback_(worker_clients);
  }

  MediaControls* CallModulesMediaControlsFactory(
      HTMLMediaElement& media_element,
      ShadowRoot& shadow_root) const {
    return media_controls_factory_(media_element, shadow_root);
  }

  void CallModulesInspectorAgentSessionInitCallback(
      InspectorSession* session,
      bool allow_view_agents,
      InspectorDOMAgent* dom_agent,
      InspectedFrames* inspected_frames,
      Page* page) const {
    inspector_agent_session_init_callback_(session, allow_view_agents,
                                           dom_agent, inspected_frames, page);
  }

  LinkResource* CallModulesServiceWorkerLinkResourceFactory(
      HTMLLinkElement* owner) const {
    return service_worker_link_resource_factory_(owner);
  }

 protected:
  // CoreInitializer is only instantiated by subclass ModulesInitializer.
  CoreInitializer() {}

  using LocalFrameCallback = void (*)(LocalFrame&);
  using WorkerClientsCallback = void (*)(WorkerClients&);
  using MediaControlsFactory = MediaControls* (*)(HTMLMediaElement&,
                                                  ShadowRoot&);
  using InspectorAgentSessionInitCallback = void (*)(InspectorSession*,
                                                     bool,
                                                     InspectorDOMAgent*,
                                                     InspectedFrames*,
                                                     Page*);
  using LinkResourceCallback = LinkResource* (*)(HTMLLinkElement*);

  LocalFrameCallback local_frame_init_callback_;
  LocalFrameCallback chrome_client_install_supplements_callback_;
  WorkerClientsCallback worker_clients_local_file_system_callback_;
  WorkerClientsCallback worker_clients_indexed_db_callback_;
  MediaControlsFactory media_controls_factory_;
  InspectorAgentSessionInitCallback inspector_agent_session_init_callback_;
  LinkResourceCallback service_worker_link_resource_factory_;

 private:
  static CoreInitializer* instance_;
  void RegisterEventFactory();
};

}  // namespace blink

#endif  // CoreInitializer_h
