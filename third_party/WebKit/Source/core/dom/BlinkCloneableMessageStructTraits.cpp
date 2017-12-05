// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/BlinkCloneableMessageStructTraits.h"

#include <stdio.h>
#include "platform/blob/BlobData.h"
#include "platform/runtime_enabled_features.h"

namespace mojo {

Vector<blink::mojom::blink::SerializedBlobPtr> StructTraits<
    blink::mojom::blink::CloneableMessage::DataView,
    blink::BlinkCloneableMessage>::blobs(blink::BlinkCloneableMessage& input) {
  Vector<blink::mojom::blink::SerializedBlobPtr> result;
  if (blink::RuntimeEnabledFeatures::MojoBlobsEnabled()) {
    result.ReserveInitialCapacity(input.message->BlobDataHandles().size());
    for (const auto& blob : input.message->BlobDataHandles()) {
      result.push_back(blink::mojom::blink::SerializedBlob::New(
          blob.value->Uuid(), blob.value->GetType(), blob.value->size(),
          blob.value->CloneBlobPtr()));
    }
  }
  return result;
}

Vector<blink::mojom::blink::SerializedArrayBufferContentsPtr>
StructTraits<blink::mojom::blink::CloneableMessage::DataView,
             blink::BlinkCloneableMessage>::
    sharedArrayBufferContentsArray(blink::BlinkCloneableMessage& input) {
  Vector<blink::mojom::blink::SerializedArrayBufferContentsPtr> result;
  //    result.ReserveInitialCapacity(input.message->SharedArrayBufferContents()->size());
  printf("about to build sharedArrayBufferContentsArray\n");
  //      printf("BlinkCloneableMessageStructTraits about to build
  //      sharedArrayBufferContentsArray. back shared?, location, holder %u, %p,
  //      %p\n", input.message->SharedArrayBuffers()->back()->IsShared(),
  //      input.message->SharedArrayBuffers()->back(),
  //      input.message->SharedArrayBuffers()->back()->DataMaybeShared());
  WTF::Vector<WTF::ArrayBufferContents, 1>* sabs =
      input.message->SharedArrayBuffers();
  for (WTF::ArrayBufferContents* it = sabs->begin(); it != sabs->end(); ++it) {
    printf("creating loc at %p\n", it);
    uint64_t loc = reinterpret_cast<uint64_t>(it);
    result.push_back(
        blink::mojom::blink::SerializedArrayBufferContents::New(loc));
  }
  //      std::transform(
  //                  sabs->begin(),
  //                  sabs->end(),
  //                  result.begin(),
  //                  [](WTF::ArrayBufferContents* contents) ->
  //                  blink::mojom::blink::SerializedArrayBufferContentsPtr {
  //      uint64_t loc = reinterpret_cast<uint64_t>(contents);
  //      return blink::mojom::blink::SerializedArrayBufferContents::New(loc);
  //      });
  //    for (const auto contents : input.message->SharedArrayBuffers()) {
  ////      printf("BlinkCloneableMessageStructTraits building
  ///sharedArrayBufferContentsArray. shared? %u\n", contents->IsShared());
  //      uint64_t loc = reinterpret_cast<uint64_t>(&contents);
  //      result.push_back(blink::mojom::blink::SerializedArrayBufferContents::New(loc));
  //    }
  return result;
}

bool StructTraits<blink::mojom::blink::CloneableMessage::DataView,
                  blink::BlinkCloneableMessage>::
    Read(blink::mojom::blink::CloneableMessage::DataView data,
         blink::BlinkCloneableMessage* out) {
  mojo::ArrayDataView<uint8_t> message_data;
  data.GetEncodedMessageDataView(&message_data);
  out->message = blink::SerializedScriptValue::Create(
      reinterpret_cast<const char*>(message_data.data()), message_data.size());

  printf("BlinkCloneableMessageStructTraits Read\n");
  Vector<blink::mojom::blink::SerializedBlobPtr> blobs;
  if (!data.ReadBlobs(&blobs))
    return false;
  Vector<blink::mojom::blink::SerializedArrayBufferContentsPtr>
      sharedArrayBufferContentsArray;
  //  blink::mojom::blink::SerializedArrayBufferContentsArrayPtr
  //  sharedArrayBufferContentsArray;
  printf(
      "BlinkCloneableMessageStructTraits Read calling "
      "ReadSharedArrayBufferContentsArray\n");
  if (!data.ReadSharedArrayBufferContentsArray(&sharedArrayBufferContentsArray))
    return false;
  printf(
      "BlinkCloneableMessageStructTraits Read sharedArrayBufferContentsArray "
      "size: %zu\n",
      sharedArrayBufferContentsArray.size());
  out->message->SharedArrayBuffers()->Grow(
      sharedArrayBufferContentsArray.size());
  size_t i = 0;
  for (auto& contents : sharedArrayBufferContentsArray) {
    //        WTF::ArrayBufferContents* h =
    //        reinterpret_cast<WTF::ArrayBufferContents*>(contents->location);
    WTF::ArrayBufferContents* c =
        reinterpret_cast<WTF::ArrayBufferContents*>(contents->location);
    //        printf("BlinkCloneableMessageStructTraits pushing back handle.
    //        allocation base: %p\n", h->AllocationBase());
    c->ShareWith(out->message->SharedArrayBuffers()->at(i++));
    //    out->message->AddSharedArrayBufferContents(*c);
  }

  return true;
}

}  // namespace mojo
