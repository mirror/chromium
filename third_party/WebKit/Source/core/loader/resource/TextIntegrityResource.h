// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextIntegrityResource_h
#define TextIntegrityResource_h

#include "core/CoreExport.h"
#include "core/loader/resource/TextResource.h"
#include "platform/loader/fetch/IntegrityMetadata.h"

namespace blink {

class Document;

// A TextResource with support for subresource integrity checking.
class CORE_EXPORT TextIntegrityResource : public TextResource {
 public:
  // SourceText provides cached access to DecodedText. It will delete the source
  // bytes from the network, and should thus be called after
  // CheckResourceIntegrity.
  const String& SourceText();

  // Check whether this resource is vali against IntegrityMetadata().
  void CheckResourceIntegrity(Document&);

  void SetIntegrityMetadataFrom(const FetchParameters&);

  // Determine the result of the integrity checking.
  ResourceIntegrityDisposition IntegrityDisposition() const {
    return integrity_disposition_;
  }

 protected:
  TextIntegrityResource(const ResourceRequest&,
                        Type,
                        const ResourceLoaderOptions&,
                        const TextResourceDecoderOptions&);
  ~TextIntegrityResource() override;

  void ClearSourceText();  // For error handling.
  size_t SourceTextSizeInBytes() const;

 private:
  // The argument must never be |NotChecked|.
  void SetIntegrityDisposition(ResourceIntegrityDisposition);

  AtomicString source_text_;
  ResourceIntegrityDisposition integrity_disposition_;
};

}  // namespace blink

#endif  // TextIntegrityResource_h
