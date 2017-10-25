// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PresentationAvailabilityState_h
#define PresentationAvailabilityState_h

#include <memory>
#include <set>
#include <vector>

#include "modules/presentation/PresentationAvailabilityCallbacks.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/Vector.h"
#include "public/platform/modules/presentation/presentation.mojom-blink.h"

namespace blink {

class PresentationAvailabilityObserver;

// Maintains the states of PresentationAvailability objects in a frame. It is
// also responsible for querying the availability values from
// PresentationService for the URLs given in a PresenationRequest. As an
// optimization, the result will be cached and shared with other
// PresentationRequests containing the same URL. PresentationAvailabilityState
// is owned by PresentationController in the same frame.
class PresentationAvailabilityState {
 public:
  explicit PresentationAvailabilityState(mojom::blink::PresentationService*);
  virtual ~PresentationAvailabilityState();

  // Requests availability for the given URLs and invokes the given callbacks
  // with the determined availability value. The callbacks will only be invoked
  // once and will be deleted afterwards.
  void AddAvailabilityCallbacks(
      const Vector<KURL>&,
      std::unique_ptr<PresentationAvailabilityCallbacks>);

  // Starts/stops listening for availability with the given observer.
  void StartListeningForAvailability(PresentationAvailabilityObserver*);
  void StopListeningForAvailability(PresentationAvailabilityObserver*);

  // Updates the availability value for a given URL, and invoking any affected
  // callbacks and observers.
  void OnScreenAvailabilityUpdated(const KURL&,
                                   mojom::blink::ScreenAvailability);

 private:
  enum class ListeningState {
    INACTIVE,
    WAITING,
    ACTIVE,
  };

  // Tracks listeners of presentation displays availability for
  // |availability_urls|. Shared with PresentationRequest objects with the same
  // set of URLs.
  struct AvailabilityListener {
    explicit AvailabilityListener(const Vector<KURL>& availability_urls);
    ~AvailabilityListener();

    const Vector<KURL> urls;
    std::vector<std::unique_ptr<PresentationAvailabilityCallbacks>>
        availability_callbacks;
    std::set<PresentationAvailabilityObserver*> availability_observers;
  };

  // Tracks listening status of |availability_url|.
  struct ListeningStatus {
    explicit ListeningStatus(const KURL& availability_url);
    ~ListeningStatus();

    const KURL url;
    mojom::blink::ScreenAvailability last_known_availability;
    ListeningState listening_state;
  };

  void StartListeningToURL(const KURL&);
  void MaybeStopListeningToURL(const KURL&);

  virtual void DoListenForScreenAvailability(const KURL&);
  virtual void DoStopListeningForScreenAvailability(const KURL&);

  mojom::blink::ScreenAvailability GetScreenAvailability(
      const Vector<KURL>&) const;
  AvailabilityListener* GetAvailabilityListener(const Vector<KURL>&) const;
  void TryRemoveAvailabilityListener(AvailabilityListener*);
  ListeningStatus* GetListeningStatus(const KURL&) const;

  // ListeningStatus for known URLs.
  std::vector<std::unique_ptr<ListeningStatus>> availability_listening_status_;

  // Set of AvailabilityListener for known PresentationRequests.
  std::vector<std::unique_ptr<AvailabilityListener>> availability_listeners_;

  // A Mojo ptr to PresentationService owned by PresentationController.
  mojom::blink::PresentationService* const presentation_service_;
};

}  // namespace blink

#endif  // PresentationAvailabilityState_h
