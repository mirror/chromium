// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ClipboardClient_h
#define ClipboardClient_h

#include <stdint.h>

#include <string>

#include "core/CoreExport.h"
#include "third_party/WebKit/common/clipboard/clipboard.mojom-blink.h"

class SkBitmap;

namespace blink {

class DataObject;

class CORE_EXPORT ClipboardClient {
 public:
  ClipboardClient();
  ~ClipboardClient() = default;

  // WebClipboard methods:
  uint64_t SequenceNumber(mojom::ClipboardBuffer);
  bool IsFormatAvailable(mojom::ClipboardFormat, mojom::ClipboardBuffer);
  Vector<String> ReadAvailableTypes(mojom::ClipboardBuffer,
                                    bool* contains_filenames);
  String ReadPlainText(mojom::ClipboardBuffer);
  String ReadHTML(mojom::ClipboardBuffer,
                  KURL* source_url,
                  unsigned* fragment_start,
                  unsigned* fragment_end);
  String ReadRTF(mojom::ClipboardBuffer);
  bool ReadImage(mojom::ClipboardBuffer,
                 String* blob_uuid,
                 String* mime_type,
                 int64_t* size);
  String ReadCustomData(mojom::ClipboardBuffer, const String& type);
  void WritePlainText(const String& plain_text);
  void WriteHTML(const String& html_text,
                 const KURL& source_url,
                 const String& plain_text,
                 bool write_smart_paste);
  void WriteImage(const SkBitmap&, const KURL& source_url, const String& title);
  void WriteDataObject(const DataObject&);

 private:
  bool IsValidBufferType(mojom::ClipboardBuffer);
  bool WriteImageToClipboard(mojom::ClipboardBuffer, const SkBitmap&);
  mojom::blink::ClipboardHostPtr clipboard_;
};

}  // namespace blink

#endif  // ClipboardClient_h
