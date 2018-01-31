// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CompositorMutatorImpl_h
#define CompositorMutatorImpl_h

#include <algorithm>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "platform/graphics/CompositorAnimator.h"
#include "platform/graphics/CompositorMutator.h"
#include "platform/heap/Handle.h"
#include "platform/heap/HeapAllocator.h"
#include "platform/wtf/RefCounted.h"
#include "public/platform/Platform.h"

namespace blink {

class CompositorMutatorClient;

// Fans out requests from the compositor to all of the registered
// CompositorAnimators which can then mutate layers through their respective
// mutate interface. Requests for animation frames are received from
// CompositorAnimators and sent to the compositor to generate a new compositor
// frame.
//
// Unless otherwise noted, this should be accessed only on the compositor
// thread.
class PLATFORM_EXPORT CompositorMutatorImpl final
    : public CompositorMutator,
      public WTF::RefCounted<
          CompositorMutatorImpl,
          base::DefaultRefCountedThreadSafeTraits<CompositorMutatorImpl>> {
 public:
  static std::unique_ptr<CompositorMutatorClient> CreateClientOnMainThread();

  // CompositorMutator implementation.
  void Mutate(std::unique_ptr<CompositorMutatorInputState>) override;

  // Interface for use by the AnimationWorklet Thread(s)
  void RegisterCompositorAnimatorOnWorkletThread(CompositorAnimator*);
  void UnregisterCompositorAnimatorOnWorkletThread(CompositorAnimator*);
  void SetMutationUpdateOnWorkletThread(
      std::unique_ptr<CompositorMutatorOutputState>);

  void SetClient(CompositorMutatorClient* client) { client_ = client; }

  ~CompositorMutatorImpl();

  base::WeakPtr<CompositorMutatorImpl> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
    // We call this on a thread different from the dereferencer.
    // We know that this is not destroyed during the call, and the
    // WeakPtr is not dereferenced except on the other thread.
  }

 private:
  CompositorMutatorImpl();

  WebThread* mutator_thread_;

  void RegisterCompositorAnimatorOnMutatorThread(CompositorAnimator*);
  void UnregisterCompositorAnimatorOnMutatorThread(CompositorAnimator*);

  // The AnimationWorkletProxyClientImpls are owned by the WorkerClients
  // dictionary.
  using CompositorAnimators =
      HashSet<CrossThreadPersistent<CompositorAnimator>>;
  CompositorAnimators animators_;

  // The MutatorClient is owned by the LayerTreeHostImpl, which also indirectly
  // own us, so this pointer is valid as long as this class exists.
  CompositorMutatorClient* client_;
  std::vector<std::unique_ptr<CompositorMutatorOutputState>> outputs_;

  base::WeakPtrFactory<CompositorMutatorImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CompositorMutatorImpl);
};

}  // namespace blink

#endif  // CompositorMutatorImpl_h
