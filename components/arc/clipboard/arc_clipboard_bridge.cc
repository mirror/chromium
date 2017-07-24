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

  switch (clip_data->content->which()) {
    case mojom::ClipDataContent::Tag::HTML:
      ProcessHTML(clip_data);
      break;
    case mojom::ClipDataContent::Tag::PLAIN_TEXT:
      ProcessPlainText(clip_data);
      break;
  }
}

void ArcClipboardBridge::ProcessHTML(const mojom::ClipDataPtr& clip_data) {
  ui::ScopedClipboardWriter writer(ui::CLIPBOARD_TYPE_COPY_PASTE);
  const mojom::ClipDataHTMLPtr& html = clip_data->content->get_html();
  writer.WriteText(base::UTF8ToUTF16(html->text));
  writer.WriteHTML(base::UTF8ToUTF16(html->markup), html->url);
}

void ArcClipboardBridge::ProcessPlainText(const mojom::ClipDataPtr& clip_data) {
  ui::ScopedClipboardWriter writer(ui::CLIPBOARD_TYPE_COPY_PASTE);
  writer.WriteText(base::UTF8ToUTF16(clip_data->content->get_plain_text()));
}

void ArcClipboardBridge::GetClipContent(
    const GetClipContentCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();

  // Order is important.
  if (clipboard->IsFormatAvailable(ui::Clipboard::GetHtmlFormatType(),
                                   ui::CLIPBOARD_TYPE_COPY_PASTE)) {
    SendHTML(clipboard, callback);
  } else if (clipboard->IsFormatAvailable(
                 ui::Clipboard::GetPlainTextFormatType(),
                 ui::CLIPBOARD_TYPE_COPY_PASTE)) {
    SendText(clipboard, callback);
  }
}

void ArcClipboardBridge::SendHTML(ui::Clipboard* clipboard,
                                  const GetClipContentCallback& callback) {
  base::string16 markup;
  std::string text, url;
  uint32_t fragment_start, fragment_end;

  clipboard->ReadHTML(ui::CLIPBOARD_TYPE_COPY_PASTE, &markup, &url,
                      &fragment_start, &fragment_end);
  clipboard->ReadAsciiText(ui::CLIPBOARD_TYPE_COPY_PASTE, &text);

  mojom::ClipDataPtr data(mojom::ClipData::New());
  mojom::ClipDataHTMLPtr html_ptr(
      mojom::ClipDataHTML::New(UTF16ToUTF8(markup), text, url));
  data->content = mojom::ClipDataContent::NewHtml(std::move(html_ptr));

  callback.Run(std::move(data));
}

void ArcClipboardBridge::SendText(ui::Clipboard* clipboard,
                                  const GetClipContentCallback& callback) {
  std::string text;

  clipboard->ReadAsciiText(ui::CLIPBOARD_TYPE_COPY_PASTE, &text);

  mojom::ClipDataPtr data(mojom::ClipData::New());
  data->content = mojom::ClipDataContent::NewPlainText(text);

  callback.Run(std::move(data));
}

}  // namespace arc
