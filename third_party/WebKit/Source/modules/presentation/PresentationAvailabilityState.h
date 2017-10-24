// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PresentationAvailabilityState_h
#define PresentationAvailabilityState_h

#include <memory>
#include <set>
#include <vector>

#include "platform/weborigin/KURL.h"
#include "platform/wtf/Vector.h"
#include "public/platform/modules/presentation/presentation.mojom-blink.h"

namespace blink {

class PresentationAvailabilityCallbacks;
class PresentationAvailabilityObserver;

class PresentationAvailabilityState {
 public:
  explicit PresentationAvailabilityState(mojom::blink::PresentationService*);
  ~PresentationAvailabilityState();

  void AddAvailabilityCallbacks(
      const Vector<KURL>&,
      std::unique_ptr<PresentationAvailabilityCallbacks>);
  void StartListeningForAvailability(PresentationAvailabilityObserver*);
  void StopListeningForAvailability(PresentationAvailabilityObserver*);

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
  mojom::blink::ScreenAvailability GetScreenAvailability(
      const Vector<KURL>&) const;
  AvailabilityListener* GetAvailabilityListener(const Vector<KURL>&) const;
  void TryRemoveAvailabilityListener(AvailabilityListener*);
  ListeningStatus* GetListeningStatus(const KURL&) const;

  // ListeningStatus for known URLs.
  std::vector<std::unique_ptr<ListeningStatus>> availability_listening_status_;

  // Set of AvailabilityListener for known PresentationRequests.
  std::vector<std::unique_ptr<AvailabilityListener>> availability_listeners_;

  mojom::blink::PresentationService* const presentation_service_;
};

}  // namespace blink

#endif  // PresentationAvailabilityState_h
