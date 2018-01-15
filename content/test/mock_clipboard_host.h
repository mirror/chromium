

#ifndef CONTENT_TEST_CLIPBOARD_HOST_H_
#define CONTENT_TEST_CLIPBOARD_HOST_H_

#include "base/strings/nullable_string16.h"
#include "base/strings/string16.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "third_party/WebKit/common/clipboard/clipboard.mojom.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace content {

class MockClipboardHost : public blink::mojom::ClipboardHost {
 public:

  MockClipboardHost();
  ~MockClipboardHost() override;

  void Bind(mojo::ScopedMessagePipeHandle handle) {
    LOG(INFO) << __PRETTY_FUNCTION__;
    bindings_.AddBinding(this, blink::mojom::ClipboardHostRequest(std::move(handle)));
  }

 private:

  struct ClipboardData {
    ClipboardData();
    ~ClipboardData();
  base::NullableString16 plain_text_;
  base::NullableString16 html_text_;
  GURL url_;
  SkBitmap bitmap_;
  std::map<base::string16, base::string16> custom_data_;
  bool write_smart_paste_;
  };

  // content::mojom::ClipboardHost
  void GetSequenceNumber(ui::ClipboardType clipboard_type,
                         GetSequenceNumberCallback callback) override;
  void IsFormatAvailable(blink::mojom::ClipboardFormat format,
                         ui::ClipboardType clipboard_type,
                         IsFormatAvailableCallback callback) override;
  void ReadAvailableTypes(ui::ClipboardType clipboard_type,
                          ReadAvailableTypesCallback callback) override;
  void ReadText(ui::ClipboardType clipboard_type,
                ReadTextCallback callback) override;
  void ReadHtml(ui::ClipboardType clipboard_type,
                ReadHtmlCallback callback) override;
  void ReadRtf(ui::ClipboardType clipboard_type,
               ReadRtfCallback callback) override;
  void ReadImage(ui::ClipboardType clipboard_type,
                 ReadImageCallback callback) override;
  void ReadCustomData(ui::ClipboardType clipboard_type,
                      const base::string16& type,
                      ReadCustomDataCallback callback) override;
  void WriteText(ui::ClipboardType clipboard_type,
                 const base::string16& text) override;
  void WriteHtml(ui::ClipboardType clipboard_type,
                 const base::string16& markup,
                 const GURL& url) override;
  void WriteSmartPasteMarker(ui::ClipboardType clipboard_type) override;
  void WriteCustomData(
      ui::ClipboardType clipboard_type,
      const std::unordered_map<base::string16, base::string16>& data) override;
  void WriteBookmark(ui::ClipboardType clipboard_type,
                     const std::string& url,
                     const base::string16& title) override;
  void WriteImage(ui::ClipboardType clipboard_type,
                  const gfx::Size& size_in_pixels,
                  mojo::ScopedSharedBufferHandle shared_buffer_handle) override;
  void CommitWrite(ui::ClipboardType clipboard_type) override;
  void WriteStringToFindPboard(const base::string16& text) override;

  void ClearClipboardData(ClipboardData& clipboard_data);

  mojo::BindingSet<blink::mojom::ClipboardHost> bindings_;
  uint64_t sequence_number_;
  ClipboardData pending_data_;
  ClipboardData written_data_;
};

}

#endif // CONTENT_TEST_CLIPBOARD_HOST_H_
