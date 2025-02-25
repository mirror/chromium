// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/blob/testing/FakeBlobRegistry.h"

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "platform/blob/testing/FakeBlob.h"

namespace blink {

void FakeBlobRegistry::Register(mojom::blink::BlobRequest blob,
                                const String& uuid,
                                const String& content_type,
                                const String& content_disposition,
                                Vector<mojom::blink::DataElementPtr> elements,
                                RegisterCallback callback) {
  registrations.push_back(Registration{uuid, content_type, content_disposition,
                                       std::move(elements)});
  mojo::MakeStrongBinding(std::make_unique<FakeBlob>(uuid), std::move(blob));
  std::move(callback).Run();
}

void FakeBlobRegistry::GetBlobFromUUID(mojom::blink::BlobRequest blob,
                                       const String& uuid,
                                       GetBlobFromUUIDCallback callback) {
  binding_requests.push_back(BindingRequest{uuid});
  mojo::MakeStrongBinding(std::make_unique<FakeBlob>(uuid), std::move(blob));
  std::move(callback).Run();
}

void FakeBlobRegistry::URLStoreForOrigin(
    const scoped_refptr<const SecurityOrigin>& origin,
    mojom::blink::BlobURLStoreAssociatedRequest request) {
  NOTREACHED();
}

}  // namespace blink
