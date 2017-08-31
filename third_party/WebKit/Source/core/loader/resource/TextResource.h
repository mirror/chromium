// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextResource_h
#define TextResource_h

#include <memory>
#include "core/CoreExport.h"
#include "core/html/parser/TextResourceDecoder.h"
#include "platform/loader/fetch/Resource.h"
#include "platform/loader/fetch/TextResourceDecoderOptions.h"

namespace blink {

class CORE_EXPORT TextResource : public Resource {
 public:
  WTF::TextEncoding Encoding() const override;

  void SetEncodingForTest(const String& encoding) { SetEncoding(encoding); }

 protected:
  TextResource(const ResourceRequest&,
               Type,
               const ResourceLoaderOptions&,
               const TextResourceDecoderOptions&);
  ~TextResource() override;

  // Returns a copy of the data received so far in text form.
  // If the resource hasn't completed loading yet, this will be an incomplete
  // representation of the resource.
  String CurrentDecodedText() const;

  // Returns the resource data in text form. This ensures that only one copy
  // and text conversion happen. This will delete the original data.
  // Requires that the resource has loaded.
  // These methods are protected so that subclasses have full control over
  // whether and when these operations are performed.
  const String& CachedDecodedText() const;
  const String& CachedDecodedText();
  void ClearDecodedTextCache();
  size_t DecodedTextCacheSize() const;

  void SetEncoding(const String&) override;

 private:
  std::unique_ptr<TextResourceDecoder> decoder_;
  String decoded_text_cache_;
};

}  // namespace blink

#endif  // TextResource_h
