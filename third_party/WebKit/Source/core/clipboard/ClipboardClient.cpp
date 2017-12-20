// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/Source/core/clipboard/ClipboardClient.h"

#include "build/build_config.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "public/platform/Platform.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/WebKit/Source/core/clipboard/DataObject.h"
#include "third_party/WebKit/Source/platform/clipboard/ClipboardMimeTypes.h"
#include "third_party/WebKit/public/platform/WebSize.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace blink {

namespace {

struct DragData {
  WTF::String text;
  WTF::String html;
  WTF::HashMap<WTF::String, WTF::String> custom_data;
};

// This plagiarizes the logic in DropDataBuilder::Build, but only extracts the
// data needed for the implementation of WriteDataObject.
// TODO(slangley): Use a mojo struct to send web_drag_data and allow receiving
// side to extract the data required.
DragData BuildDragData(const DataObject& drag_data) {
  DragData result;

  for (size_t i = 0; i < drag_data.length(); ++i) {
    const DataObjectItem* const item = drag_data.Item(i);
    if (item->Kind() == DataObjectItem::kStringKind) {
      if (item->GetType() == blink::kMimeTypeText) {
        result.text = item->GetAsString();
      } else if (item->GetType() == blink::kMimeTypeTextHTML) {
        result.html = item->GetAsString();
      } else if (item->GetType() != blink::kMimeTypeTextURIList &&
                 item->GetType() != blink::kMimeTypeDownloadURL) {
        result.custom_data.insert(item->GetType(), item->GetAsString());
      }
    }
  }

  return result;
};

#if !defined(OS_MACOSX)
// TODO(slangley): crbug.com/775830 Remove the implementation of
// URLToImageMarkup from clipboard_utils.h once we can delete
// MockClipboardClient.
WTF::String URLToImageMarkup(const KURL& url, const WTF::String& title) {
  WTF::String markup("<img src=\"");
  markup.append(EncodeWithURLEscapeSequences(url.GetString()));
  markup.append("\"");
  if (!title.IsEmpty()) {
    markup.append(" alt=\"");
    markup.append(EncodeWithURLEscapeSequences(title));
    markup.append("\"");
  }
  markup.append("/>");
  return markup;
}
#endif

String EnsureNotNullWTFString(const String& string) {
  if (string.IsNull()) {
    return g_empty_string16_bit;
  }
  return string;
}

}  // namespace

ClipboardClient::ClipboardClient() {
  LOG(INFO) << __PRETTY_FUNCTION__ << " - "
            << Platform::Current()->GetBrowserServiceName();
  Platform::Current()->GetConnector()->BindInterface(
      Platform::Current()->GetBrowserServiceName(), &clipboard_);
}

uint64_t ClipboardClient::SequenceNumber(mojom::ClipboardBuffer buffer) {
  uint64_t result = 0;
  if (!IsValidBufferType(buffer))
    return 0;

  clipboard_->GetSequenceNumber(buffer, &result);
  return result;
}

bool ClipboardClient::IsFormatAvailable(mojom::ClipboardFormat format,
                                        mojom::ClipboardBuffer buffer) {
  if (!IsValidBufferType(buffer))
    return false;

  bool result = false;
  clipboard_->IsFormatAvailable(format, buffer, &result);
  return result;
}

Vector<String> ClipboardClient::ReadAvailableTypes(
    mojom::ClipboardBuffer buffer,
    bool* contains_filenames) {
  Vector<String> types;
  if (IsValidBufferType(buffer)) {
    clipboard_->ReadAvailableTypes(buffer, &types, contains_filenames);
  }
  return types;
}

String ClipboardClient::ReadPlainText(mojom::ClipboardBuffer buffer) {
  if (!IsValidBufferType(buffer))
    return String();

  WTF::String text;
  clipboard_->ReadText(buffer, &text);
  return text;
}

String ClipboardClient::ReadHTML(mojom::ClipboardBuffer buffer,
                                 KURL* source_url,
                                 unsigned* fragment_start,
                                 unsigned* fragment_end) {
  DCHECK(source_url);
  if (!IsValidBufferType(buffer))
    return String();

  String html_stdstr;
  clipboard_->ReadHtml(buffer, &html_stdstr, source_url,
                       static_cast<uint32_t*>(fragment_start),
                       static_cast<uint32_t*>(fragment_end));
  return html_stdstr;
}

String ClipboardClient::ReadRTF(mojom::ClipboardBuffer buffer) {
  if (!IsValidBufferType(buffer))
    return String();

  String rtf;
  clipboard_->ReadRtf(buffer, &rtf);
  return rtf;
}

bool ClipboardClient::ReadImage(mojom::ClipboardBuffer buffer,
                                String* blob_uuid,
                                String* mime_type,
                                int64_t* size) {
  if (!IsValidBufferType(buffer))
    return false;

  return clipboard_->ReadImage(buffer, blob_uuid, mime_type, size);
}

String ClipboardClient::ReadCustomData(mojom::ClipboardBuffer buffer,
                                       const String& type) {
  if (!IsValidBufferType(buffer))
    return String();

  WTF::String data;
  clipboard_->ReadCustomData(buffer, EnsureNotNullWTFString(type), &data);
  return data;
}

void ClipboardClient::WritePlainText(const String& plain_text) {
  clipboard_->WriteText(mojom::ClipboardBuffer::kStandard, plain_text);
  clipboard_->CommitWrite(mojom::ClipboardBuffer::kStandard);
}

void ClipboardClient::WriteHTML(const String& html_text,
                                const KURL& source_url,
                                const String& plain_text,
                                bool write_smart_paste) {
  clipboard_->WriteHtml(mojom::ClipboardBuffer::kStandard, html_text,
                        source_url);
  clipboard_->WriteText(mojom::ClipboardBuffer::kStandard, plain_text);

  if (write_smart_paste)
    clipboard_->WriteSmartPasteMarker(mojom::ClipboardBuffer::kStandard);
  clipboard_->CommitWrite(mojom::ClipboardBuffer::kStandard);
}

void ClipboardClient::WriteImage(const SkBitmap& image,
                                 const KURL& url,
                                 const String& title) {
  if (!WriteImageToClipboard(mojom::ClipboardBuffer::kStandard, image))
    return;

  if (url.IsValid() && !url.IsEmpty()) {
    clipboard_->WriteBookmark(mojom::ClipboardBuffer::kStandard,
                              url.GetString(), EnsureNotNullWTFString(title));
#if !defined(OS_MACOSX)
    // When writing the image, we also write the image markup so that pasting
    // into rich text editors, such as Gmail, reveals the image. We also don't
    // want to call writeText(), since some applications (WordPad) don't pick
    // the image if there is also a text format on the clipboard.
    // We also don't want to write HTML on a Mac, since Mail.app prefers to use
    // the image markup over attaching the actual image. See
    // http://crbug.com/33016 for details.
    clipboard_->WriteHtml(mojom::ClipboardBuffer::kStandard,
                          URLToImageMarkup(url, title), KURL());
#endif
  }
  clipboard_->CommitWrite(mojom::ClipboardBuffer::kStandard);
}

void ClipboardClient::WriteDataObject(const DataObject& data) {
  const DragData& drag_data = BuildDragData(data);
  // TODO(dcheng): Properly support text/uri-list here.
  // Avoid calling the WriteFoo functions if there is no data associated with a
  // type. This prevents stomping on clipboard contents that might have been
  // written by extension functions such as chrome.bookmarkManagerPrivate.copy.
  if (!drag_data.text.IsNull()) {
    clipboard_->WriteText(mojom::ClipboardBuffer::kStandard, drag_data.text);
  }
  if (!drag_data.html.IsNull()) {
    clipboard_->WriteHtml(mojom::ClipboardBuffer::kStandard, drag_data.html,
                          KURL());
  }
  if (!drag_data.custom_data.IsEmpty()) {
    clipboard_->WriteCustomData(mojom::ClipboardBuffer::kStandard,
                                std::move(drag_data.custom_data));
  }
  clipboard_->CommitWrite(mojom::ClipboardBuffer::kStandard);
}

bool ClipboardClient::IsValidBufferType(mojom::ClipboardBuffer buffer) {
  switch (buffer) {
    case mojom::ClipboardBuffer::kStandard:
      return true;
    case mojom::ClipboardBuffer::kSelection:
#if defined(USE_X11)
      return true;
#else
      // Chrome OS and non-X11 unix builds do not support
      // the X selection clipboad.
      // TODO: remove the need for this case, see http://crbug.com/361753
      return false;
#endif
  }
  return true;
}

bool ClipboardClient::WriteImageToClipboard(mojom::ClipboardBuffer buffer,
                                            const SkBitmap& bitmap) {
  // Only 32-bit bitmaps are supported.
  DCHECK_EQ(bitmap.colorType(), kN32_SkColorType);

  const WebSize size(bitmap.width(), bitmap.height());

  void* pixels = bitmap.getPixels();
  // TODO(piman): this should not be NULL, but it is. crbug.com/369621
  if (!pixels)
    return false;

  base::CheckedNumeric<uint32_t> checked_buf_size = 4;
  checked_buf_size *= size.width;
  checked_buf_size *= size.height;
  if (!checked_buf_size.IsValid())
    return false;

  // Allocate a shared memory buffer to hold the bitmap bits.
  uint32_t buf_size = checked_buf_size.ValueOrDie();
  auto shared_buffer = mojo::SharedBufferHandle::Create(buf_size);
  auto mapping = shared_buffer->Map(buf_size);
  memcpy(mapping.get(), pixels, buf_size);

  clipboard_->WriteImage(buffer, size, std::move(shared_buffer));
  return true;
}

}  // namespace blink
