#include "content/test/mock_clipboard_host.h"

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"

namespace content {

MockClipboardHost::MockClipboardHost() = default;

MockClipboardHost::~MockClipboardHost() = default;

void MockClipboardHost::GetSequenceNumber(ui::ClipboardType clipboard_type,
                       GetSequenceNumberCallback callback) {
  LOG(INFO) << "*** " << __PRETTY_FUNCTION__;
  std::move(callback).Run(sequence_number_);
}

void MockClipboardHost::IsFormatAvailable(blink::mojom::ClipboardFormat format,
                       ui::ClipboardType clipboard_type,
                       IsFormatAvailableCallback callback) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  bool result = false;
  switch (format) {
    case blink::mojom::ClipboardFormat::kPlaintext:
      LOG(INFO) << "Checking plain text";
      result = !written_data_.plain_text_.is_null();
      break;
    case blink::mojom::ClipboardFormat::kHtml:
      LOG(INFO) << "Checking html text";
      result = !written_data_.html_text_.is_null();
      break;
    case blink::mojom::ClipboardFormat::kSmartPaste:
      LOG(INFO) << "Checking smart paste";
      result = written_data_.write_smart_paste_;
      break;
    case blink::mojom::ClipboardFormat::kBookmark:
      break;
  }
  LOG(INFO) << "$$$ Result = " << result;
  std::move(callback).Run(result);
}

void MockClipboardHost::ReadAvailableTypes(ui::ClipboardType clipboard_type,
                        ReadAvailableTypesCallback callback) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  constexpr bool kContainsFilenames = false;
  std::vector<base::string16> types;

  if (!written_data_.plain_text_.string().empty()) {
    LOG(INFO) << "$$$ - Plain text available.";
    types.push_back(base::UTF8ToUTF16("text/plain"));
  }
  if (!written_data_.html_text_.string().empty()) {
    LOG(INFO) << "$$$ - Html text available.";
    types.push_back(base::UTF8ToUTF16("text/html"));
  }
  if (!written_data_.bitmap_.isNull()) {
    LOG(INFO) << "$$$ - Image available.";
    types.push_back(base::UTF8ToUTF16("image/png"));
  }
  for(const auto& data : written_data_.custom_data_) {
    LOG(INFO) << "$$$ - Custom available.";
    types.push_back(data.first);
  }

  std::move(callback).Run(types, kContainsFilenames);
}

void MockClipboardHost::ReadText(ui::ClipboardType clipboard_type,
              ReadTextCallback callback) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  std::move(callback).Run(written_data_.plain_text_.string());
}

void MockClipboardHost::ReadHtml(ui::ClipboardType clipboard_type,
              ReadHtmlCallback callback) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  std::move(callback).Run(written_data_.html_text_.string(),
                          written_data_.url_,
                          0,
                          written_data_.html_text_.string().length());
}

void MockClipboardHost::ReadRtf(ui::ClipboardType clipboard_type,
             ReadRtfCallback callback) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  NOTREACHED();
}

void MockClipboardHost::ReadImage(ui::ClipboardType clipboard_type,
               ReadImageCallback callback) {
  LOG(INFO) << __PRETTY_FUNCTION__;
}

void MockClipboardHost::ReadCustomData(ui::ClipboardType clipboard_type,
                    const base::string16& type,
                    ReadCustomDataCallback callback) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  const auto it = written_data_.custom_data_.find(type);
  if (it != written_data_.custom_data_.end()) {
    std::move(callback).Run(it->second);
  }
  NOTREACHED();
  std::move(callback).Run(base::UTF8ToUTF16(""));
}

void MockClipboardHost::WriteText(ui::ClipboardType clipboard_type,
               const base::string16& text) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " - " << text;
  pending_data_.plain_text_ = base::NullableString16(text, false);
  ++sequence_number_;
}

void MockClipboardHost::WriteHtml(ui::ClipboardType clipboard_type,
               const base::string16& markup,
               const GURL& url) {
  LOG(INFO) << __PRETTY_FUNCTION__ << " - " << markup;
  pending_data_.html_text_ = base::NullableString16(markup, false);
  pending_data_.url_ = url;
  ++sequence_number_;
}

void MockClipboardHost::WriteSmartPasteMarker(ui::ClipboardType clipboard_type) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  pending_data_.write_smart_paste_ = true;
}

void MockClipboardHost::WriteCustomData(
    ui::ClipboardType clipboard_type,
    const std::unordered_map<base::string16, base::string16>& data) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  pending_data_.custom_data_.insert(data.begin(), data.end());
}

void MockClipboardHost::WriteBookmark(ui::ClipboardType clipboard_type,
                   const std::string& url,
                   const base::string16& title) {
  LOG(INFO) << __PRETTY_FUNCTION__;
}

void MockClipboardHost::WriteImage(ui::ClipboardType clipboard_type,
                const gfx::Size& size_in_pixels,
                mojo::ScopedSharedBufferHandle shared_buffer_handle) {
  LOG(INFO) << __PRETTY_FUNCTION__;
}

void MockClipboardHost::CommitWrite(ui::ClipboardType clipboard_type) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  written_data_ = pending_data_;
  ClearClipboardData(pending_data_);
}

void MockClipboardHost::WriteStringToFindPboard(const base::string16& text) {
  LOG(INFO) << __PRETTY_FUNCTION__;
  NOTREACHED();
}

void MockClipboardHost::ClearClipboardData(ClipboardData& data) {
  data.plain_text_ = base::NullableString16();
  data.html_text_ = base::NullableString16();
  data.url_ = GURL();
  data.bitmap_.reset();
  data.custom_data_.clear();
  data.write_smart_paste_ = false;
}

MockClipboardHost::ClipboardData::ClipboardData() = default;
MockClipboardHost::ClipboardData::~ClipboardData() = default;

}
