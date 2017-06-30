// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/clipboard/arc_clipboard_bridge.h"

#include <sys/types.h>
#include <unistd.h>

#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_checker.h"
#include "chrome/browser/image_decoder.h"
#include "components/arc/arc_bridge_service.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/system/core.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_monitor.h"
#include "ui/base/clipboard/clipboard_types.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/size.h"

namespace arc {
namespace {
const uint64_t kSeqNotInitialized = -1;

std::vector<uint8_t> EncodeImagePng(const SkBitmap& bitmap) {
  std::vector<uint8_t> output;

  if (!gfx::PNGCodec::FastEncodeBGRASkBitmap(bitmap, true, &output)) {
    LOG(ERROR) << "Error Encoding SkBitmap to PNG";
  }

  return output;
}

base::Optional<base::FilePath> EncodeImagePngAndSaveToDisk(
    const SkBitmap& bitmap) {
  std::vector<uint8_t> output;

  if (!gfx::PNGCodec::FastEncodeBGRASkBitmap(bitmap, true, &output)) {
    LOG(ERROR) << "Error Encoding SkBitmap to PNG";
    return base::nullopt;
  }

  base::FilePath file_path;
  if (!base::CreateTemporaryFile(&file_path)) {
    LOG(ERROR) << "Error creating temporary file";
    return base::nullopt;
  }

  base::File out(file_path,
                 base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);

  int written =
      out.WriteAtCurrentPos(reinterpret_cast<char*>(&output[0]), output.size());
  if (written < 0) {
    LOG(ERROR) << "Error creating temporary file";
    return base::nullopt;
  }
  return file_path;
}
}  // namespace

class ArcClipboardBridge::DecodeRequest : public ImageDecoder::ImageRequest {
 public:
  DecodeRequest(ArcClipboardBridge* service, int32_t android_id)
      : service_(service), android_id_(android_id) {}
  ~DecodeRequest() override = default;

  // ImageDecoder::ImageRequest overrides.
  void OnImageDecoded(const SkBitmap& bitmap) override {
    DCHECK(service_->CalledOnValidThread());
    ui::ScopedClipboardWriter writer(ui::CLIPBOARD_TYPE_COPY_PASTE);
    writer.WriteImage(bitmap);
  }

  void OnDecodeImageFailed() override {
    DCHECK(service_->CalledOnValidThread());
    DLOG(ERROR) << "Failed to decode wallpaper image.";
  }

 private:
  // ArcWallpaperService owns DecodeRequest, so it will outlive this.
  ArcClipboardBridge* const service_;
  const int32_t android_id_;

  DISALLOW_COPY_AND_ASSIGN(DecodeRequest);
};

ArcClipboardBridge::ArcClipboardBridge(ArcBridgeService* bridge_service)
    : ArcService(bridge_service),
      binding_(this),
      host_clip_version_(0),
      instance_clip_version_(0),
      clipboard_seq_from_instance_(kSeqNotInitialized) {
  arc_bridge_service()->clipboard()->AddObserver(this);
  ui::ClipboardMonitor::GetInstance()->AddObserver(this);
}

ArcClipboardBridge::~ArcClipboardBridge() {
  DCHECK(CalledOnValidThread());
  arc_bridge_service()->clipboard()->RemoveObserver(this);
}

void ArcClipboardBridge::OnInstanceReady() {
  mojom::ClipboardInstance* clipboard_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service()->clipboard(), Init);
  DCHECK(clipboard_instance);
  mojom::ClipboardHostPtr host_proxy;
  binding_.Bind(mojo::MakeRequest(&host_proxy));
  clipboard_instance->Init(std::move(host_proxy));
}

void ArcClipboardBridge::OnClipboardDataChanged() {
  DCHECK(CalledOnValidThread());

  LOG(ERROR) << "ArcClipboardBridge::OnClipboardDataChanged()";

  // quick return. WIP
  return;

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  if (!clipboard)
    return;

  uint64_t current_seq =
      clipboard->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE);
  if (clipboard_seq_from_instance_ == kSeqNotInitialized ||
      current_seq > clipboard_seq_from_instance_ + 1) {
    host_clip_version_++;
    mojom::ClipboardInstance* clipboard_instance = ARC_GET_INSTANCE_FOR_METHOD(
        arc_bridge_service()->clipboard(), HostClipboardUpdated);
    if (!clipboard_instance)
      return;
    clipboard_instance->HostClipboardUpdated();
  } else {
    // ignore this event, since this event was triggered by the Instance, and
    // not by the Host
  }
}

void ArcClipboardBridge::SetTextContent(const std::string& text) {
  DCHECK(CalledOnValidThread());
  ui::ScopedClipboardWriter writer(ui::CLIPBOARD_TYPE_COPY_PASTE);
  writer.WriteText(base::UTF8ToUTF16(text));
}

void ArcClipboardBridge::GetTextContent() {
  DCHECK(CalledOnValidThread());

  base::string16 text;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadText(ui::CLIPBOARD_TYPE_COPY_PASTE, &text);

  mojom::ClipboardInstance* clipboard_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service()->clipboard(), OnGetTextContent);
  if (!clipboard_instance)
    return;
  clipboard_instance->OnGetTextContent(base::UTF16ToUTF8(text));
}

void ArcClipboardBridge::SetHostClip(mojom::ClipDataPtr clip_data) {
  DCHECK(CalledOnValidThread());

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  if (!clipboard)
    return;
  clipboard_seq_from_instance_ =
      clipboard->GetSequenceNumber(ui::CLIPBOARD_TYPE_COPY_PASTE);

  ui::ScopedClipboardWriter writer(ui::CLIPBOARD_TYPE_COPY_PASTE);

  if (clip_data->content->is_html()) {
    const mojom::ClipDataHTMLPtr& html = clip_data->content->get_html();
    writer.WriteHTML(base::UTF8ToUTF16(html->markup), html->url);
  } else if (clip_data->content->is_opaque_data()) {
    if (clip_data->mime_type == "image/png") {
      // 1: decode PNG, and convert it to SkBitmap in another process
      // 2: update clipboard with SkBitmap
      const std::vector<uint8_t>& data = clip_data->content->get_opaque_data();
      decode_request_ = base::MakeUnique<DecodeRequest>(this, 0);
      ImageDecoder::StartWithOptions(decode_request_.get(), data,
                                     ImageDecoder::DEFAULT_CODEC, true,
                                     gfx::Size());
    }
  } else if (clip_data->content->is_file()) {
    mojo::ScopedHandle& handle = clip_data->content->get_file();
    if (!handle.is_valid()) {
      LOG(ERROR) << "Invalid handle";
      return;
    }

    if (clip_data->mime_type == "image/png") {
      // 1: read fd into tmp buffer
      // 2: decode PNG, and convert it to SkBitmap in another process
      // 3: update clipboard with SkBitmap
      // 4: release buffer

      // Previous request will be cancelled at destructor of
      // ImageDecoder::ImageRequest.
      mojo::edk::ScopedPlatformHandle scoped_platform_handle;
      MojoResult mojo_result = mojo::edk::PassWrappedPlatformHandle(
          handle.release().value(), &scoped_platform_handle);
      if (mojo_result != MOJO_RESULT_OK) {
        LOG(ERROR) << "PassWrappedPlatformHandle failed: " << mojo_result;
        return;
      }

      const int fd = scoped_platform_handle.release().handle;
      off_t file_size = lseek(fd, (size_t)0, SEEK_END);
      lseek(fd, (size_t)0, SEEK_SET);
      std::vector<uint8_t> data;
      data.reserve(file_size);
      if (!base::ReadFromFD(fd, reinterpret_cast<char*>(&data[0]), file_size)) {
        LOG(ERROR) << "ReadFromFD failed";
        return;
      }
      decode_request_ = base::MakeUnique<DecodeRequest>(this, 0);
      ImageDecoder::StartWithOptions(decode_request_.get(), data,
                                     ImageDecoder::DEFAULT_CODEC, true,
                                     gfx::Size());
    }
  } else {
    writer.WriteText(base::UTF8ToUTF16(clip_data->content->get_plain_text()));
  }
}

void ArcClipboardBridge::GetHostClip() {
  LOG(ERROR) << "GetHostClip() -- ENTER";

  DCHECK(CalledOnValidThread());

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();

  if (clipboard->IsFormatAvailable(ui::Clipboard::GetBitmapFormatType(),
                                   ui::CLIPBOARD_TYPE_COPY_PASTE)) {
    LOG(ERROR) << "GetHostClip() -- 1";
    SkBitmap bitmap = clipboard->ReadImage(ui::CLIPBOARD_TYPE_COPY_PASTE);
    // ConvertAndSendBitmapToInstance(bitmap);
    ConvertBitmapAndSendFdToInstance(bitmap);
  } else if (clipboard->IsFormatAvailable(ui::Clipboard::GetHtmlFormatType(),
                                          ui::CLIPBOARD_TYPE_COPY_PASTE)) {
    LOG(ERROR) << "GetHostClip() -- 2";
    base::string16 markup;
    std::string text;
    std::string src_url;
    uint32_t fragment_start, fragment_end;
    clipboard->ReadHTML(ui::CLIPBOARD_TYPE_COPY_PASTE, &markup, &src_url,
                        &fragment_start, &fragment_end);
    clipboard->ReadAsciiText(ui::CLIPBOARD_TYPE_COPY_PASTE, &text);
    SendHTMLToInstance(src_url, base::UTF16ToUTF8(markup), text);
  } else if (clipboard->IsFormatAvailable(
                 ui::Clipboard::GetPlainTextFormatType(),
                 ui::CLIPBOARD_TYPE_COPY_PASTE)) {
    LOG(ERROR) << "GetHostClip() -- 3";
    std::string text;
    clipboard->ReadAsciiText(ui::CLIPBOARD_TYPE_COPY_PASTE, &text);
    SendTextToInstance(text);
  } else {
    LOG(ERROR) << "GetHostClip() -- 4";
    SendEmptyToInstance();
  }

  LOG(ERROR) << "GetHostClip() -- LEAVE";
}

void ArcClipboardBridge::ConvertAndSendBitmapToInstance(
    const SkBitmap& bitmap) {
  LOG(ERROR) << "ConvertAndSendImageToInstance() -- ENTER";

  DCHECK(CalledOnValidThread());

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&EncodeImagePng, bitmap),
      base::Bind(&ArcClipboardBridge::SendImageToInstance,
                 base::Unretained(this)));

  LOG(ERROR) << "ConvertAndSendImageToInstance() -- LEAVE";
}

void ArcClipboardBridge::ConvertBitmapAndSendFdToInstance(
    const SkBitmap& bitmap) {
  LOG(ERROR) << "ConvertBitmapAndSendFdToInstance() -- ENTER";

  DCHECK(CalledOnValidThread());

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&EncodeImagePngAndSaveToDisk, bitmap),
      base::Bind(&ArcClipboardBridge::SendFileToInstance,
                 base::Unretained(this), "image/png"));

  LOG(ERROR) << "ConvertBitmapAndSendFdToInstance() -- LEAVE";
}

void ArcClipboardBridge::SendFileToInstance(
    const std::string& mime_type,
    base::Optional<base::FilePath> file_path) {
  LOG(ERROR) << "SendFileToInstance() -- ENTER";

  if (!file_path.has_value())
    return;

  mojom::ClipboardInstance* clipboard_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service()->clipboard(), OnGetHostClip);
  if (!clipboard_instance)
    return;

  base::File file(file_path.value(), base::File::FLAG_READ |
                                         base::File::FLAG_OPEN |
                                         base::File::FLAG_DELETE_ON_CLOSE);
  int fd = file.GetPlatformFile();
  if (fd == -1) {
    LOG(ERROR) << "GetPlatformFile() returned invalid fd";
    return;
  }

  MojoHandle wrapped_handle;
  MojoResult wrap_result = mojo::edk::CreatePlatformHandleWrapper(
      mojo::edk::ScopedPlatformHandle(mojo::edk::PlatformHandle(fd)),
      &wrapped_handle);
  if (wrap_result != MOJO_RESULT_OK) {
    LOG(ERROR) << "Failed to wrap handles. " << wrap_result;
    return;
  }
  mojo::ScopedHandle scoped_handle{mojo::Handle(wrapped_handle)};

  mojom::ClipDataPtr data(mojom::ClipData::New());
  data->content = mojom::ClipDataContent::NewFile(std::move(scoped_handle));
  data->mime_type = mime_type;
  clipboard_instance->OnGetHostClip(std::move(data));

  // queue file
  files_to_delete_.push_back(std::move(file));

  LOG(ERROR) << "SendFileToInstance() -- LEAVE";
}

void ArcClipboardBridge::SendImageToInstance(
    const std::vector<uint8_t>& png_bytes) {
  LOG(ERROR) << "SendImageToInstance() -- ENTER";

  mojom::ClipboardInstance* clipboard_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service()->clipboard(), OnGetHostClip);
  if (!clipboard_instance)
    return;

  mojom::ClipDataPtr data(mojom::ClipData::New());
  data->content = mojom::ClipDataContent::NewOpaqueData(png_bytes);
  data->mime_type = "image/png";
  clipboard_instance->OnGetHostClip(std::move(data));

  LOG(ERROR) << "SendImageToInstance() -- LEAVE";
}

void ArcClipboardBridge::SendHTMLToInstance(const std::string& url,
                                            const std::string& markup,
                                            const std::string& text) {
  LOG(ERROR) << "SendHTMLToInstance() -- ENTER";

  mojom::ClipboardInstance* clipboard_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service()->clipboard(), OnGetHostClip);
  if (!clipboard_instance)
    return;

  mojom::ClipDataPtr data(mojom::ClipData::New());
  mojom::ClipDataHTMLPtr html_ptr(mojom::ClipDataHTML::New(markup, text, url));
  data->content = mojom::ClipDataContent::NewHtml(std::move(html_ptr));
  data->mime_type = "text/html";

  clipboard_instance->OnGetHostClip(std::move(data));

  LOG(ERROR) << "SendHTMLToInstance() -- LEAVE";
}

void ArcClipboardBridge::SendTextToInstance(const std::string& clip_text) {
  LOG(ERROR) << "SendTextToInstance() -- ENTER";

  mojom::ClipboardInstance* clipboard_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service()->clipboard(), OnGetHostClip);
  if (!clipboard_instance)
    return;

  mojom::ClipDataPtr data(mojom::ClipData::New());
  data->content = mojom::ClipDataContent::NewPlainText(clip_text);
  data->mime_type = "text/plain";
  clipboard_instance->OnGetHostClip(std::move(data));

  LOG(ERROR) << "SendTextToInstance() -- LEAVE";
}

void ArcClipboardBridge::SendEmptyToInstance() {
  LOG(ERROR) << "SendEmptyToInstance() -- ENTER";

  mojom::ClipboardInstance* clipboard_instance = ARC_GET_INSTANCE_FOR_METHOD(
      arc_bridge_service()->clipboard(), OnGetHostClip);

  if (!clipboard_instance)
    return;

  mojom::ClipDataPtr data(mojom::ClipData::New());
  data->content = mojom::ClipDataContent::NewPlainText("<empty>");
  clipboard_instance->OnGetHostClip(std::move(data));

  LOG(ERROR) << "SendEmptyToInstance() -- LEAVE";
}

bool ArcClipboardBridge::CalledOnValidThread() {
  // Make sure access to the Chrome clipboard is happening in the UI thread.
  return thread_checker_.CalledOnValidThread();
}

}  // namespace arc
