// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_PROTOCOL_DEVTOOLS_DOWNLOAD_ITEM_H_
#define CONTENT_BROWSER_DEVTOOLS_PROTOCOL_DEVTOOLS_DOWNLOAD_ITEM_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "net/base/io_buffer.h"

#include "content/browser/download/download_create_info.h"
#include "content/browser/download/download_destination_observer.h"
#include "content/browser/download/download_item_impl.h"
#include "content/browser/download/download_request_handle.h"
#include "content/common/content_export.h"
#include "content/public/browser/download_interrupt_reasons.h"
#include "content/public/browser/download_item.h"
#include "content/public/common/download_stream.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "net/log/net_log_with_source.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
class DownloadManager;
class DownloadRequestHandleInterface;
class DownloadSourceStream;

namespace protocol {

namespace {
// Data length to read from data pipe.
const int kBytesToRead = 4096;

}  //  namespace.

class CONTENT_EXPORT DevToolsDownloadItem : public content::DownloadItem {
 public:
  // Constructing for a regular download.
  // |net_log| is constructed externally for our use.
  DevToolsDownloadItem(const content::DownloadCreateInfo& info);

  using ReadCompleteCallback = base::OnceCallback<void(size_t)>;

  ~DevToolsDownloadItem() override;

  // Initialize the download associated to a |stream|.
  // |req_handle| is the new request handle associated with the download.
  void Initialize(std::unique_ptr<DownloadManager::InputStream> stream,
                  std::unique_ptr<DownloadRequestHandleInterface> req_handle);

  // Read from the download stream.
  // |data| is a reference to a read buffer, which has already been initialized
  // to store |bytes_to_read| bytes.
  // |bytes_to_read| indicates how many bytes we want to read from the stream.
  // |callback| will be invoked when the data is already ready to read from
  // the buffer.
  void Read(scoped_refptr<net::IOBuffer> data,
            size_t bytes_to_read,
            ReadCompleteCallback callback);

  // Implemented DownloadItem functions
  void Cancel(bool user_cancel) override;
  void Remove() override;
  const std::string& GetGuid() const override;
  DownloadItem::DownloadState GetState() const override;
  content::DownloadInterruptReason GetLastReason() const override;
  bool IsDone() const override;
  const GURL& GetURL() const override;
  const std::vector<GURL>& GetUrlChain() const override;
  const GURL& GetOriginalUrl() const override;
  const GURL& GetReferrerUrl() const override;
  const GURL& GetSiteUrl() const override;
  const GURL& GetTabUrl() const override;
  const GURL& GetTabReferrerUrl() const override;
  std::string GetSuggestedFilename() const override;
  const scoped_refptr<const net::HttpResponseHeaders>& GetResponseHeaders()
      const override;
  std::string GetContentDisposition() const override;
  std::string GetMimeType() const override;
  std::string GetOriginalMimeType() const override;
  std::string GetRemoteAddress() const override;
  bool HasUserGesture() const override;
  ui::PageTransition GetTransitionType() const override;
  const std::string& GetLastModifiedTime() const override;
  const std::string& GetETag() const override;
  int64_t GetTotalBytes() const override;
  base::Time GetStartTime() const override;
  content::BrowserContext* GetBrowserContext() const override;
  content::WebContents* GetWebContents() const override;
  std::string DebugString(bool verbose) const override;
  bool AllDataSaved() const override;
  void SimulateErrorForTesting(DownloadInterruptReason reason) override;

  // Not implemented DownloadItem functions
  void AddObserver(DownloadItem::Observer* observer) override{};
  void RemoveObserver(DownloadItem::Observer* observer) override{};
  void UpdateObservers() override{};
  void ValidateDangerousDownload() override{};
  void StealDangerousDownload(bool need_removal,
                              const AcquireFileCallback& callback) override{};
  void Pause() override{};
  void Resume() override{};
  void OpenDownload() override{};
  void ShowDownloadInShell() override{};
  uint32_t GetId() const override;

  bool IsPaused() const override;
  bool IsTemporary() const override;
  bool CanResume() const override;
  bool IsSavePackageDownload() const override;
  const base::FilePath& GetFullPath() const override;
  const base::FilePath& GetTargetFilePath() const override;
  const base::FilePath& GetForcedFilePath() const override;
  base::FilePath GetFileNameToReportUser() const override;
  DownloadItem::TargetDisposition GetTargetDisposition() const override;
  const std::string& GetHash() const override;
  bool GetFileExternallyRemoved() const override;
  void DeleteFile(const base::Callback<void(bool)>& callback) override;
  bool IsDangerous() const override;
  content::DownloadDangerType GetDangerType() const override;
  bool TimeRemaining(base::TimeDelta* remaining) const override;
  int64_t CurrentSpeed() const override;
  int PercentComplete() const override;
  int64_t GetReceivedBytes() const override;
  const std::vector<DownloadItem::ReceivedSlice>& GetReceivedSlices()
      const override;
  base::Time GetEndTime() const override;
  bool CanShowInFolder() override;
  bool CanOpenDownload() override;
  bool ShouldOpenFileBasedOnExtension() override;
  bool GetOpenWhenComplete() const override;
  bool GetAutoOpened() override;
  bool GetOpened() const override;
  base::Time GetLastAccessTime() const override;
  bool IsTransient() const override;
  void OnContentCheckCompleted(
      content::DownloadDangerType danger_type,
      content::DownloadInterruptReason reason) override{};
  void SetOpenWhenComplete(bool open) override{};
  void SetOpened(bool opened) override{};
  void SetLastAccessTime(base::Time last_access_time) override{};
  void SetDisplayName(const base::FilePath& name) override{};

 private:
  void DataAvailable(MojoResult result);
  void ReadInternal();
  void TransitionTo(DownloadItem::DownloadState new_state);
  void InterruptWithReason(content::DownloadInterruptReason reason);
  static const char* DebugDownloadStateString(
      DownloadItem::DownloadState state);

  bool reading_;
  scoped_refptr<net::IOBuffer> current_buffer_read_;
  scoped_refptr<net::DrainableIOBuffer> pending_write_buffer_;
  scoped_refptr<net::IOBuffer> incoming_data_;
  size_t current_buffer_size_;
  size_t current_offset_;
  int current_bytes_remaining_;
  ReadCompleteCallback callback_;

  content::DownloadItemImpl::RequestInfo info_;

  // Stream associated to the download.
  std::unique_ptr<DownloadSourceStream> stream_;

  // Current download item associated request handle. Assigned when
  // the download has started
  std::unique_ptr<DownloadRequestHandleInterface> request_handle_;

  // GUID to identify the download, generated by |base::GenerateGUID| in
  // download item, or provided by |DownloadUrlParameters|.
  // The format should follow UUID version 4 in RFC 4122.
  // The string representation is case sensitive. Legacy download GUID hex
  // digits may be upper case ASCII characters, and new GUID will be in lower
  // case.
  std::string guid_;

  // The HTTP response headers. This contains a nullptr when the response has
  // not yet been received. Only for consuming headers.
  scoped_refptr<const net::HttpResponseHeaders> response_headers_;

  // Content-disposition field from the header.
  std::string content_disposition_;

  // Mime-type from the header.  Subject to change.
  std::string mime_type_;

  // The value of the content type header sent with the downloaded item.  It
  // may be different from |mime_type_|, which may be set based on heuristics
  // which may look at the file extension and first few bytes of the file.
  std::string original_mime_type_;

  // Last reason.
  content::DownloadInterruptReason last_reason_ =
      DOWNLOAD_INTERRUPT_REASON_NONE;

  // Total bytes expected.
  int64_t total_bytes_ = 0;

  // Contents of the Last-Modified header for the most recent server response.
  std::string last_modified_time_;

  // Server's ETAG for the file.
  std::string etag_;

  // The current state of this download.
  DownloadItem::DownloadState state_ = IN_PROGRESS;

  const base::FilePath empty_path_;
  std::vector<DownloadItem::ReceivedSlice> empty_slices_;
  const std::string empty_hash_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<DevToolsDownloadItem> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsDownloadItem);
};

}  // namespace protocol
}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_PROTOCOL_DEVTOOLS_DOWNLOAD_ITEM_H_
