// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/presentation/PresentationController.h"

#include <memory>
#include "core/dom/Document.h"
#include "core/frame/Deprecation.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/UseCounter.h"
#include "modules/presentation/PresentationAvailabilityCallbacks.h"
#include "modules/presentation/PresentationAvailabilityObserver.h"
#include "modules/presentation/PresentationConnection.h"
#include "platform/wtf/PtrUtil.h"
#include "public/platform/Platform.h"
#include "public/platform/WebString.h"
#include "public/platform/WebVector.h"
#include "public/platform/modules/presentation/WebPresentationClient.h"
#include "public/platform/modules/presentation/WebPresentationError.h"

namespace blink {

PresentationController::PresentationController(LocalFrame& frame,
                                               WebPresentationClient* client)
    : Supplement<LocalFrame>(frame),
      ContextLifecycleObserver(frame.GetDocument()),
      client_(client),
      controller_binding_(this) {}

PresentationController::~PresentationController() {
}

// static
PresentationController* PresentationController::Create(
    LocalFrame& frame,
    WebPresentationClient* client) {
  return new PresentationController(frame, client);
}

// static
const char* PresentationController::SupplementName() {
  return "PresentationController";
}

// static
PresentationController* PresentationController::From(LocalFrame& frame) {
  return static_cast<PresentationController*>(
      Supplement<LocalFrame>::From(frame, SupplementName()));
}

// static
void PresentationController::ProvideTo(LocalFrame& frame,
                                       WebPresentationClient* client) {
  Supplement<LocalFrame>::ProvideTo(
      frame, PresentationController::SupplementName(),
      PresentationController::Create(frame, client));
}

WebPresentationClient* PresentationController::Client() {
  return client_;
}

// static
PresentationController* PresentationController::FromContext(
    ExecutionContext* execution_context) {
  if (!execution_context)
    return nullptr;

  DCHECK(execution_context->IsDocument());
  Document* document = ToDocument(execution_context);
  if (!document->GetFrame())
    return nullptr;

  return PresentationController::From(*document->GetFrame());
}

// static
WebPresentationClient* PresentationController::ClientFromContext(
    ExecutionContext* execution_context) {
  PresentationController* controller =
      PresentationController::FromContext(execution_context);
  return controller ? controller->Client() : nullptr;
}

DEFINE_TRACE(PresentationController) {
  visitor->Trace(presentation_);
  visitor->Trace(connections_);
  Supplement<LocalFrame>::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

void PresentationController::SetPresentation(Presentation* presentation) {
  presentation_ = presentation;
}

void PresentationController::SetDefaultRequestUrl(const Vector<KURL>& urls) {
  auto& presentation_service = GetPresentationService();
  DCHECK(presentation_service);
  presentation_service->SetDefaultPresentationUrls(urls);
}

void PresentationController::RegisterConnection(
    ControllerPresentationConnection* connection) {
  connections_.insert(connection);
}

void PresentationController::GetAvailability(
    const Vector<KURL>& urls,
    std::unique_ptr<PresentationAvailabilityCallbacks> callback) {
  auto screen_availability = GetScreenAvailability(urls);
  // Reject Promise if screen availability is unsupported for all URLs.
  if (screen_availability == mojom::blink::ScreenAvailability::DISABLED) {
    Platform::Current()->CurrentThread()->GetWebTaskRunner()->PostTask(
        FROM_HERE,
        WTF::Bind(
            &PresentationAvailabilityCallbacks::OnError, std::move(callback),
            blink::WebPresentationError(
                blink::WebPresentationError::kErrorTypeAvailabilityNotSupported,
                "Screen availability monitoring not supported")));
    // Do not listen to urls if we reject the promise.
    return;
  }

  auto* listener = GetAvailabilityListener(urls);
  if (!listener) {
    listener = new AvailabilityListener(urls);
    availability_listeners_.emplace_back(listener);
  }

  if (screen_availability != mojom::blink::ScreenAvailability::UNKNOWN) {
    Platform::Current()->CurrentThread()->GetWebTaskRunner()->PostTask(
        FROM_HERE, WTF::Bind(&PresentationAvailabilityCallbacks::OnSuccess,
                             std::move(callback),
                             screen_availability ==
                                 mojom::blink::ScreenAvailability::AVAILABLE));
  } else {
    listener->availability_callbacks.push_back(std::move(callback));
  }

  // This is an optimization in case the page decides to listen for
  // availability.
  for (const auto& availability_url : urls)
    StartListeningToURL(availability_url);
}

void PresentationController::StartListeningForAvailability(
    PresentationAvailabilityObserver* observer) {
  const auto& urls = observer->Urls();
  auto* listener = GetAvailabilityListener(urls);
  if (!listener) {
    listener = new AvailabilityListener(urls);
    availability_listeners_.emplace_back(listener);
  }

  listener->availability_observers.insert(observer);
  for (const auto& availability_url : urls)
    StartListeningToURL(availability_url);
}

void PresentationController::StopListeningForAvailability(
    PresentationAvailabilityObserver* observer) {
  const auto& urls = observer->Urls();
  auto* listener = GetAvailabilityListener(urls);
  if (!listener) {
    DLOG(WARNING) << "Stop listening for availability for unknown URLs.";
    return;
  }

  listener->availability_observers.erase(observer);
  for (const auto& availability_url : urls)
    MaybeStopListeningToURL(availability_url);

  TryRemoveAvailabilityListener(listener);
}

void PresentationController::OnScreenAvailabilityUpdated(
    const KURL& url,
    mojom::blink::ScreenAvailability availability) {
  auto* listening_status = GetListeningStatus(url);
  if (!listening_status)
    return;

  if (listening_status->listening_state == ListeningState::WAITING)
    listening_status->listening_state = ListeningState::ACTIVE;

  if (listening_status->last_known_availability == availability)
    return;

  listening_status->last_known_availability = availability;

  std::vector<AvailabilityListener*> modified_listeners;
  for (auto& listener : availability_listeners_) {
    if (!listener->urls.Contains<KURL>(url))
      continue;

    auto screen_availability = GetScreenAvailability(listener->urls);
    DCHECK(screen_availability != mojom::blink::ScreenAvailability::UNKNOWN);
    for (auto* observer : listener->availability_observers)
      observer->AvailabilityChanged(screen_availability);

    if (screen_availability == mojom::blink::ScreenAvailability::DISABLED) {
      static const WebString& not_supported_error = blink::WebString::FromUTF8(
          "getAvailability() isn't supported at the moment. It can be due to "
          "a permanent or temporary system limitation. It is recommended to "
          "try to blindly start a presentation in that case.");
      for (auto& callback_ptr : listener->availability_callbacks) {
        callback_ptr->OnError(blink::WebPresentationError(
            blink::WebPresentationError::kErrorTypeAvailabilityNotSupported,
            not_supported_error));
      }
    } else {
      for (auto& callback_ptr : listener->availability_callbacks) {
        callback_ptr->OnSuccess(screen_availability ==
                                mojom::blink::ScreenAvailability::AVAILABLE);
      }
    }
    listener->availability_callbacks.clear();

    for (const auto& availability_url : listener->urls)
      MaybeStopListeningToURL(availability_url);

    modified_listeners.push_back(listener.get());
  }

  for (auto* listener : modified_listeners)
    TryRemoveAvailabilityListener(listener);
}

void PresentationController::OnConnectionStateChanged(
    mojom::blink::PresentationInfoPtr presentation_info,
    mojom::blink::PresentationConnectionState state) {
  PresentationConnection* connection = FindConnection(*presentation_info);
  if (!connection)
    return;

  connection->DidChangeState(state);
}

void PresentationController::OnConnectionClosed(
    mojom::blink::PresentationInfoPtr presentation_info,
    mojom::blink::PresentationConnectionCloseReason reason,
    const String& message) {
  PresentationConnection* connection = FindConnection(*presentation_info);
  if (!connection)
    return;

  connection->DidClose(reason, message);
}

void PresentationController::OnDefaultPresentationStarted(
    mojom::blink::PresentationInfoPtr presentation_info) {
  DCHECK(presentation_info);
  if (!presentation_ || !presentation_->defaultRequest())
    return;

  PresentationRequest::RecordStartOriginTypeAccess(*GetExecutionContext());
  auto* connection = ControllerPresentationConnection::Take(
      this, *presentation_info, presentation_->defaultRequest());
  connection->Init();
}

void PresentationController::StartListeningToURL(const KURL& url) {
  auto* listening_status = GetListeningStatus(url);
  if (!listening_status) {
    listening_status = new ListeningStatus(url);
    availability_listening_status_.emplace_back(listening_status);
  }

  // Already listening.
  if (listening_status->listening_state != ListeningState::INACTIVE)
    return;

  listening_status->listening_state = ListeningState::WAITING;

  // XXX: check for null needed?
  auto& presentation_service = GetPresentationService();
  DCHECK(presentation_service);
  presentation_service->ListenForScreenAvailability(url);
}

void PresentationController::MaybeStopListeningToURL(const KURL& url) {
  for (const auto& listener : availability_listeners_) {
    if (!listener->urls.Contains(url))
      continue;

    // URL is still observed by some availability object.
    if (!listener->availability_callbacks.empty() ||
        !listener->availability_observers.empty()) {
      return;
    }
  }

  auto* listening_status = GetListeningStatus(url);
  if (!listening_status) {
    LOG(WARNING) << "Stop listening to unknown url: " << url.GetString();
    return;
  }

  if (listening_status->listening_state == ListeningState::INACTIVE)
    return;

  auto& presentation_service = GetPresentationService();
  DCHECK(presentation_service);
  presentation_service->StopListeningForScreenAvailability(url);
}

mojom::blink::ScreenAvailability PresentationController::GetScreenAvailability(
    const Vector<KURL>& urls) const {
  bool has_disabled = false;
  bool has_source_not_supported = false;
  bool has_unavailable = false;

  for (const auto& url : urls) {
    auto* status = GetListeningStatus(url);
    auto screen_availability = status
                                   ? status->last_known_availability
                                   : mojom::blink::ScreenAvailability::UNKNOWN;
    if (screen_availability == mojom::blink::ScreenAvailability::AVAILABLE)
      return mojom::blink::ScreenAvailability::AVAILABLE;

    if (screen_availability == mojom::blink::ScreenAvailability::DISABLED) {
      has_disabled = true;
    } else if (screen_availability ==
               mojom::blink::ScreenAvailability::SOURCE_NOT_SUPPORTED) {
      has_source_not_supported = true;
    } else if (screen_availability ==
               mojom::blink::ScreenAvailability::UNAVAILABLE) {
      has_unavailable = true;
    }
  }

  if (has_disabled)
    return mojom::blink::ScreenAvailability::DISABLED;
  if (has_source_not_supported)
    return mojom::blink::ScreenAvailability::SOURCE_NOT_SUPPORTED;
  if (has_unavailable)
    return mojom::blink::ScreenAvailability::UNAVAILABLE;
  return mojom::blink::ScreenAvailability::UNKNOWN;
}

PresentationController::AvailabilityListener*
PresentationController::GetAvailabilityListener(
    const Vector<KURL>& urls) const {
  auto listener_it = std::find_if(
      availability_listeners_.begin(), availability_listeners_.end(),
      [&urls](const std::unique_ptr<AvailabilityListener>& x) {
        return x->urls == urls;
      });
  return listener_it == availability_listeners_.end() ? nullptr
                                                      : listener_it->get();
}

void PresentationController::TryRemoveAvailabilityListener(
    AvailabilityListener* listener) {
  // URL is still observed by some availability object.
  if (!listener->availability_callbacks.empty() ||
      !listener->availability_observers.empty()) {
    return;
  }

  auto listener_it = availability_listeners_.begin();
  while (listener_it != availability_listeners_.end()) {
    if (listener_it->get() == listener) {
      availability_listeners_.erase(listener_it);
      return;
    }
    ++listener_it;
  }
}

PresentationController::ListeningStatus*
PresentationController::GetListeningStatus(const KURL& url) const {
  auto status_it =
      std::find_if(availability_listening_status_.begin(),
                   availability_listening_status_.end(),
                   [&url](const std::unique_ptr<ListeningStatus>& status) {
                     return status->url == url;
                   });
  return status_it == availability_listening_status_.end() ? nullptr
                                                           : status_it->get();
}

void PresentationController::ContextDestroyed(ExecutionContext*) {
  client_ = nullptr;
  controller_binding_.Close();
}

ControllerPresentationConnection*
PresentationController::FindExistingConnection(
    const blink::WebVector<blink::WebURL>& presentation_urls,
    const blink::WebString& presentation_id) {
  for (const auto& connection : connections_) {
    for (const auto& presentation_url : presentation_urls) {
      if (connection->GetState() !=
              mojom::blink::PresentationConnectionState::TERMINATED &&
          connection->Matches(presentation_id, presentation_url)) {
        return connection.Get();
      }
    }
  }
  return nullptr;
}

mojom::blink::PresentationServicePtr&
PresentationController::GetPresentationService() {
  if (!presentation_service_ && GetFrame() && GetFrame()->Client()) {
    auto* interface_provider = GetFrame()->Client()->GetInterfaceProvider();
    interface_provider->GetInterface(mojo::MakeRequest(&presentation_service_));

    DCHECK(!controller_binding_.is_bound());
    mojom::blink::PresentationControllerPtr controller_ptr;
    controller_binding_.Bind(mojo::MakeRequest(&controller_ptr));
    presentation_service_->SetController(std::move(controller_ptr));
  }
  return presentation_service_;
}

ControllerPresentationConnection* PresentationController::FindConnection(
    const mojom::blink::PresentationInfo& presentation_info) const {
  for (const auto& connection : connections_) {
    if (connection->Matches(presentation_info.id, presentation_info.url))
      return connection.Get();
  }

  return nullptr;
}

PresentationController::AvailabilityListener::AvailabilityListener(
    const Vector<KURL>& availability_urls)
    : urls(availability_urls) {}

PresentationController::AvailabilityListener::~AvailabilityListener() {}

PresentationController::ListeningStatus::ListeningStatus(
    const KURL& availability_url)
    : url(availability_url),
      last_known_availability(mojom::blink::ScreenAvailability::UNKNOWN),
      listening_state(ListeningState::INACTIVE) {}

PresentationController::ListeningStatus::~ListeningStatus() {}

}  // namespace blink
