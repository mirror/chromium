// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_LIB_BINDINGS_SERIALIZATION_CONTEXT_H_
#define MOJO_PUBLIC_CPP_BINDINGS_LIB_BINDINGS_SERIALIZATION_CONTEXT_H_

#include <stddef.h>

#include <memory>
#include <queue>
#include <vector>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/bindings_export.h"
#include "mojo/public/cpp/bindings/lib/bindings_internal.h"
#include "mojo/public/cpp/bindings/scoped_interface_endpoint_handle.h"
#include "mojo/public/cpp/system/handle.h"

namespace mojo {

class Message;

namespace internal {

// A container for handles during serialization/deserialization.
class MOJO_CPP_BINDINGS_EXPORT SerializedHandleVector {
 public:
  SerializedHandleVector();
  ~SerializedHandleVector();

  size_t size() const { return handles_.size(); }
  std::vector<mojo::ScopedHandle>* mutable_handles() { return &handles_; }

  size_t num_tracked_handles() const { return tracked_handles_.size(); }
  std::vector<mojo::Handle>* tracked_handles() { return &tracked_handles_; }

  // Adds a handle to the list of tracked handles. Note that unlike AddHandle,
  // this does not take ownership of the handle. Handles are accumulated during
  // the message sizing phase, and every handle tracked should correspond to an
  // eventual call to AddHandle() on this context, in the same order in which
  // the handles were tracked.
  void TrackHandle(mojo::Handle handle);

  // Serializes a handle into index data to be included in a message. |handle|
  // must correspond to a handle previously tracked by TrackHandle, and calls
  // to AddHandle should happen in the same order as calls to TrackHandle.
  Handle_Data AddHandle(mojo::ScopedHandle handle);

  // Takes a handle from the list of serialized handle data.
  mojo::ScopedHandle TakeHandle(const Handle_Data& encoded_handle);

  // Takes a handle from the list of serialized handle data and returns it in
  // |*out_handle| as a specific scoped handle type.
  template <typename T>
  ScopedHandleBase<T> TakeHandleAs(const Handle_Data& encoded_handle) {
    return ScopedHandleBase<T>::From(TakeHandle(encoded_handle));
  }

  // Swaps all owned handles out with another Handle vector.
  void Swap(std::vector<mojo::ScopedHandle>* other);

 private:
  // Handles tracked but not (yet) owned by this object.
  std::vector<mojo::Handle> tracked_handles_;

  // Handles are owned by this object.
  std::vector<mojo::ScopedHandle> handles_;

  DISALLOW_COPY_AND_ASSIGN(SerializedHandleVector);
};

// Context information for serialization/deserialization routines.
struct MOJO_CPP_BINDINGS_EXPORT SerializationContext {
  SerializationContext();

  ~SerializationContext();

  // Transfers ownership of any accumulated associated endpoint handles into
  // |*message|.
  void AttachHandlesToMessage(Message* message);

  // Opaque context pointers returned by StringTraits::SetUpContext().
  std::unique_ptr<std::queue<void*>> custom_contexts;

  // Stashes handles encoded in a message by index.
  SerializedHandleVector handles;

  // The number of ScopedInterfaceEndpointHandles that need to be serialized.
  // It is calculated by PrepareToSerialize().
  uint32_t associated_endpoint_count = 0;

  // Stashes ScopedInterfaceEndpointHandles encoded in a message by index.
  std::vector<ScopedInterfaceEndpointHandle> associated_endpoint_handles;
};

}  // namespace internal
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_LIB_BINDINGS_SERIALIZATION_CONTEXT_H_
