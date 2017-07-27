// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/clipboard/arc_clipboard_bridge.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_checker.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_monitor.h"
#include "ui/base/clipboard/clipboard_types.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

namespace arc {
namespace {

const uint64_t kSeqNotInitialized = -1;

// Singleton factory for ArcClipboardBridge.
class ArcClipboardBridgeFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcClipboardBridge,
          ArcClipboardBridgeFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcClipboardBridgeFactory";

  static ArcClipboardBridgeFactory* GetInstance() {
    return base::Singleton<ArcClipboardBridgeFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcClipboardBridgeFactory>;
  ArcClipboardBridgeFactory() = default;
  ~ArcClipboardBridgeFactory() override = default;
};

}  // namespace

// static
ArcClipboardBridge* ArcClipboardBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcClipboardBridgeFactory::GetForBrowserContext(context);
}

ArcClipboardBridge::ArcClipboardBridge(content::BrowserContext* context,
                                       ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service),
      binding_(this),
      clipboard_seq_from_instance_(kSeqNotInitialized) {
  arc_bridge_service_->clipboard()->AddObserver(this);
  ui::ClipboardMonitor::GetInstance()->AddObserver(this);
}

ArcClipboardBridge::~ArcClipboardBridge() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  arc_bridge_service_->clipboard()->RemoveObserver(this);
}

void ArcClipboardBridge::OnInstanceReady() {
  mojom::ClipboardInstance* clipboard_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->clipboard(), Init);
  DCHECK(clipboard_instance);
  mojom::ClipboardHostPtr host_proxy;
  binding_.Bind(mojo::MakeRequest(&host_proxy));
  clipboard_instance->Init(std::move(host_proxy));
}

void ArcClipboardBridge::OnClipboardDataChanged() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  if (!clipboard)
    return;

  const uint64_t current_seq =
      clipboard->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE);

  if (clipboard_seq_from_instance_ != kSeqNotInitialized &&
      current_seq <= clipboard_seq_from_instance_ + 1) {
    // ignore this event, since this event was triggered by a 'paste' in
    // Instance, and not by Host
    return;
  }

  mojom::ClipboardInstance* clipboard_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service_->clipboard(), HostClipboardUpdated);
  if (!clipboard_instance)
    return;

  // TODO(ricardoq): should only inform the Instance when a supported
  // mime_type is copied to the clipboard
  clipboard_instance->HostClipboardUpdated();
}

void ArcClipboardBridge::SetTextContentDeprecated(const std::string& text) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  ui::ScopedClipboardWriter writer(ui::CLIPBOARD_TYPE_COPY_PASTE);
  writer.WriteText(base::UTF8ToUTF16(text));
}

void ArcClipboardBridge::GetTextContentDeprecated() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  base::string16 text;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadText(ui::CLIPBOARD_TYPE_COPY_PASTE, &text);

  mojom::ClipboardInstance* clipboard_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service_->clipboard(), OnGetTextContentDeprecated);
  if (!clipboard_instance)
    return;
  clipboard_instance->OnGetTextContentDeprecated(base::UTF16ToUTF8(text));
}

void ArcClipboardBridge::SetClipContent(mojom::ClipDataPtr clip_data) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  if (!clipboard)
    return;
  clipboard_seq_from_instance_ =
      clipboard->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE);

  // reuse ScopedClipboardWriter so that all items belong to the same clip
  // and don't generate more than once sequence increment
  ui::ScopedClipboardWriter writer(ui::CLIPBOARD_TYPE_COPY_PASTE);

  for (const auto& item : clip_data->items) {
    const std::string& mime_type(item->mime_type);
    if (mime_type == ui::Clipboard::kMimeTypeHTML) {
      ProcessItemHTML(item.get(), &writer);
    } else if (mime_type == ui::Clipboard::kMimeTypeText) {
      ProcessItemPlainText(item.get(), &writer);
    }
  }
}

void ArcClipboardBridge::ProcessItemHTML(mojom::ClipItem* clip_item,
                                         ui::ScopedClipboardWriter* writer) {
  std::string html(std::begin(clip_item->data), std::end(clip_item->data));
  writer->WriteHTML(base::UTF8ToUTF16(html), "");
}

void ArcClipboardBridge::ProcessItemPlainText(
    mojom::ClipItem* clip_item,
    ui::ScopedClipboardWriter* writer) {
  std::string text(std::begin(clip_item->data), std::end(clip_item->data));
  writer->WriteText(base::UTF8ToUTF16(text));
}

void ArcClipboardBridge::GetClipContent(
    const GetClipContentCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  std::vector<base::string16> mime_types;
  bool contains_files;
  clipboard->ReadAvailableTypes(ui::CLIPBOARD_TYPE_COPY_PASTE, &mime_types,
                                &contains_files);

  mojom::ClipDataPtr clip_data(mojom::ClipData::New());

  // skip PlainText if HTML is available. HTML will be converted to PlainText on
  // the Instance side when needed
  bool skip_plain_text = clipboard->IsFormatAvailable(
      ui::Clipboard::GetHtmlFormatType(), ui::CLIPBOARD_TYPE_COPY_PASTE);

  // populate ClipData with ClipItems
  for (const auto& mime_type16 : mime_types) {
    const std::string mime_type(base::UTF16ToUTF8(mime_type16));
    if (mime_type == ui::Clipboard::kMimeTypeHTML) {
      clip_data->items.push_back(CreateItemHTML(clipboard));
    } else if (!skip_plain_text && mime_type == ui::Clipboard::kMimeTypeText) {
      clip_data->items.push_back(CreateItemPlainText(clipboard));
    }
    // add other supported mime_types here
  }

  // call callback, even if the clip is empty, since Instance is waiting for a
  // response
  callback.Run(std::move(clip_data));
}

mojom::ClipItemPtr ArcClipboardBridge::CreateItemHTML(
    ui::Clipboard* clipboard) {
  base::string16 markup16;
  std::string text, url;
  uint32_t fragment_start, fragment_end;

  clipboard->ReadHTML(ui::CLIPBOARD_TYPE_COPY_PASTE, &markup16, &url,
                      &fragment_start, &fragment_end);
  const std::string markup8(base::UTF16ToUTF8(markup16));
  const std::vector<uint8_t> data(std::begin(markup8), std::end(markup8));

  return mojom::ClipItem::New(ui::Clipboard::kMimeTypeHTML, data);
}

mojom::ClipItemPtr ArcClipboardBridge::CreateItemPlainText(
    ui::Clipboard* clipboard) {
  std::string text;
  clipboard->ReadAsciiText(ui::CLIPBOARD_TYPE_COPY_PASTE, &text);

  const std::vector<uint8_t> data(std::begin(text), std::end(text));

  return mojom::ClipItem::New(ui::Clipboard::kMimeTypeText, data);
}

}  // namespace arc
