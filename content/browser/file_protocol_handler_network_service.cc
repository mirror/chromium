// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/file_protocol_handler_network_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/resource_request.h"
#include "content/public/common/url_loader.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/file_data_pipe_producer.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "net/base/directory_lister.h"
#include "net/base/directory_listing.h"
#include "net/base/filename_util.h"
#include "net/base/mime_sniffer.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/url_request/redirect_info.h"
#include "url/gurl.h"

#if defined(OS_WIN)
#include "base/win/shortcut.h"
#endif

namespace content {
namespace {

constexpr size_t kDefaultFileUrlPipeSize = 65536;

bool IsPathAccessAllowed(const base::FilePath& path,
                         const base::FilePath& profile_path) {
  bool access_allowed =
      GetContentClient()->browser()->IsFileAccessAllowed(path, profile_path);
#if !defined(OS_ANDROID)
  // Android's whitelist relies on symbolic links (ex. /sdcard is whitelisted
  // and commonly a symbolic link), thus only check absolute paths on other
  // platforms.
  access_allowed &= GetContentClient()->browser()->IsFileAccessAllowed(
      base::MakeAbsoluteFilePath(path), profile_path);
#endif
  return access_allowed;
}

class FileURLLoader : public mojom::URLLoader {
 public:
  static void CreateAndStart(const base::FilePath& profile_path,
                             const ResourceRequest& request,
                             mojom::URLLoaderRequest loader,
                             mojom::URLLoaderClientPtrInfo client_info) {
    // Owns itself. Will live as long as its URLLoader and URLLoaderClientPtr
    // bindings are alive - essentially until either the client gives up or all
    // file data has been sent to it.
    auto* file_url_loader = new FileURLLoader;
    file_url_loader->Start(profile_path, request, std::move(loader),
                           std::move(client_info));
  }

  // mojom::URLLoader:
  void FollowRedirect() override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

 private:
  FileURLLoader() : binding_(this) {}
  ~FileURLLoader() override = default;

  void Start(const base::FilePath& profile_path,
             const ResourceRequest& request,
             mojom::URLLoaderRequest loader,
             mojom::URLLoaderClientPtrInfo client_info) {
    binding_.Bind(std::move(loader));
    binding_.set_connection_error_handler(base::BindOnce(
        &FileURLLoader::OnConnectionError, base::Unretained(this)));

    mojom::URLLoaderClientPtr client;
    client.Bind(std::move(client_info));

    base::FilePath path;
    if (!net::FileURLToFilePath(request.url, &path)) {
      client->OnComplete(ResourceRequestCompletionStatus(net::ERR_FAILED));
      return;
    }

    base::File::Info info;
    if (!base::GetFileInfo(path, &info)) {
      client->OnComplete(
          ResourceRequestCompletionStatus(net::ERR_FILE_NOT_FOUND));
      return;
    }

    bool should_redirect = false;
    net::RedirectInfo redirect_info;
    if (info.is_directory && !path.EndsWithSeparator()) {
      // If the named path is a directory with no trailing slash, redirect to
      // the same path, but with a trailing slash.
      std::string new_path = request.url.path() + '/';
      GURL::Replacements replacements;
      replacements.SetPathStr(new_path);

      should_redirect = true;
      redirect_info.new_url = request.url.ReplaceComponents(replacements);
    }
#if defined(OS_WIN)
    else if (base::LowerCaseEqualsASCII(path.Extension(), ".lnk")) {
      // Follow Windows shortcuts
      base::FilePath new_path;
      if (base::win::ResolveShortcut(path, &new_path, nullptr)) {
        should_redirect = true;
        redirect_info.new_url = net::FilePathToFileURL(new_path);
      }
    }
#endif  // defined(OS_WIN)

    if (should_redirect) {
      redirect_info.new_method = "GET";
      redirect_info.status_code = 301;
      client->OnReceiveRedirect(redirect_info, ResourceResponseHead());
      return;
    }

    if (!IsPathAccessAllowed(path, profile_path)) {
      client->OnComplete(
          ResourceRequestCompletionStatus(net::ERR_ACCESS_DENIED));
      return;
    }

    mojo::DataPipe pipe(kDefaultFileUrlPipeSize);
    if (!pipe.consumer_handle.is_valid()) {
      client->OnComplete(ResourceRequestCompletionStatus(net::ERR_FAILED));
      return;
    }

    if (info.is_directory) {
      // TODO(crbug.com/759230): Directory listing support.
      client->OnComplete(ResourceRequestCompletionStatus(net::ERR_FAILED));
      return;
    }

    base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
    char initial_buffer[net::kMaxBytesToSniff];
    int read_size =
        file.ReadAtCurrentPos(initial_buffer, net::kMaxBytesToSniff);
    if (read_size < 0) {
      client->OnComplete(ResourceRequestCompletionStatus(net::ERR_FAILED));
      return;
    }

    // This data should always fit in the pipe.
    DCHECK_LE(static_cast<size_t>(net::kMaxBytesToSniff),
              kDefaultFileUrlPipeSize);
    uint32_t write_size = static_cast<uint32_t>(read_size);
    MojoResult result = pipe.producer_handle->WriteData(
        initial_buffer, &write_size, MOJO_WRITE_DATA_FLAG_NONE);
    if (result != MOJO_RESULT_OK ||
        write_size != static_cast<uint32_t>(read_size)) {
      OnFileWritten(result);
      return;
    }

    ResourceResponseHead head;
    net::GetMimeTypeFromFile(path, &head.mime_type);
    net::SniffMimeType(initial_buffer, read_size, request.url, head.mime_type,
                       &head.mime_type);

    // TODO(crbug.com/759230): Support range requests.
    head.content_length = static_cast<int64_t>(info.size);

    client->OnReceiveResponse(head, base::nullopt, nullptr);
    client->OnStartLoadingResponseBody(std::move(pipe.consumer_handle));
    client_ = std::move(client);

    if (read_size < net::kMaxBytesToSniff) {
      // There's definitely no more data to read, so we're already done.
      OnFileWritten(MOJO_RESULT_OK);
      return;
    }

    // There may be data left in the file (its length is at least
    // |kMaxBytesToSniff|). Sending the rest of the file asynchronously.
    data_producer_ = std::make_unique<mojo::FileDataPipeProducer>(
        std::move(pipe.producer_handle));
    data_producer_->WriteFromFile(
        std::move(file),
        base::BindOnce(&FileURLLoader::OnFileWritten, base::Unretained(this)));
  }

  void OnConnectionError() {
    binding_.Close();
    MaybeDeleteSelf();
  }

  void MaybeDeleteSelf() {
    if (!binding_.is_bound() && !client_.is_bound())
      delete this;
  }

  void OnFileWritten(MojoResult result) {
    if (result == MOJO_RESULT_OK)
      client_->OnComplete(ResourceRequestCompletionStatus(net::OK));
    else
      client_->OnComplete(ResourceRequestCompletionStatus(net::ERR_FAILED));
    client_.reset();
    MaybeDeleteSelf();
  }

  std::unique_ptr<mojo::FileDataPipeProducer> data_producer_;
  mojo::Binding<mojom::URLLoader> binding_;
  mojom::URLLoaderClientPtr client_;

  DISALLOW_COPY_AND_ASSIGN(FileURLLoader);
};

class FileURLDirectoryLoader
    : public mojom::URLLoader,
      public net::DirectoryLister::DirectoryListerDelegate {
 public:
  static void CreateAndStart(const base::FilePath& profile_path,
                             const ResourceRequest& request,
                             mojom::URLLoaderRequest loader,
                             mojom::URLLoaderClientPtrInfo client_info) {
    // Owns itself. Will live as long as its URLLoader and URLLoaderClientPtr
    // bindings are alive - essentially until either the client gives up or all
    // file data has been sent to it.
    auto* file_url_loader = new FileURLDirectoryLoader;
    file_url_loader->Start(profile_path, request, std::move(loader),
                           std::move(client_info));
  }

  // mojom::URLLoader:
  void FollowRedirect() override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

 private:
  FileURLDirectoryLoader() : binding_(this) {}
  ~FileURLDirectoryLoader() override = default;

  void Start(const base::FilePath& profile_path,
             const ResourceRequest& request,
             mojom::URLLoaderRequest loader,
             mojom::URLLoaderClientPtrInfo client_info) {
    binding_.Bind(std::move(loader));
    binding_.set_connection_error_handler(base::BindOnce(
        &FileURLDirectoryLoader::OnConnectionError, base::Unretained(this)));

    mojom::URLLoaderClientPtr client;
    client.Bind(std::move(client_info));

    if (!net::FileURLToFilePath(request.url, &path_)) {
      client->OnComplete(ResourceRequestCompletionStatus(net::ERR_FAILED));
      return;
    }

    base::File::Info info;
    if (!base::GetFileInfo(path_, &info) || !info.is_directory) {
      client->OnComplete(
          ResourceRequestCompletionStatus(net::ERR_FILE_NOT_FOUND));
      return;
    }

    if (!IsPathAccessAllowed(path_, profile_path)) {
      client->OnComplete(
          ResourceRequestCompletionStatus(net::ERR_ACCESS_DENIED));
      return;
    }

    mojo::DataPipe pipe(kDefaultFileUrlPipeSize);
    if (!pipe.consumer_handle.is_valid()) {
      client->OnComplete(ResourceRequestCompletionStatus(net::ERR_FAILED));
      return;
    }

    if (info.is_directory) {
      // TODO(crbug.com/759230): Directory listing support.
      client->OnComplete(ResourceRequestCompletionStatus(net::ERR_FAILED));
      return;
    }

    ResourceResponseHead head;
    head.mime_type = "text/html";
    head.charset = "utf-8";
    client->OnReceiveResponse(head, base::nullopt, nullptr);
    client->OnStartLoadingResponseBody(std::move(pipe.consumer_handle));
    client_ = std::move(client);

    lister_ = std::make_unique<net::DirectoryLister>(path_, this);
    lister_->Start();

    data_producer_ = std::make_unique<mojo::StringDataPipeProducer>(
        std::move(pipe.producer_handle));
  }

  void OnConnectionError() {
    binding_.Close();
    MaybeDeleteSelf();
  }

  void MaybeDeleteSelf() {
    if (!binding_.is_bound() && !client_.is_bound() && !lister_)
      delete this;
  }

  // net::DirectoryLister::DirectoryListerDelegate:
  void OnListFile(
      const net::DirectoryLister::DirectoryListerData& data) override {
    if (!wrote_header_) {
      wrote_header_ = true;

#if defined(OS_WIN)
      const base::string16& title = path_.value();
#elif defined(OS_POSIX)
      const base::string16& title =
          base::WideToUTF16(base::SysNativeMBToWide(path_.value()));
#endif
      pending_data_.append(net::GetDirectoryListingHeader(title));

      // If not a top-level directory, add a link to the parent directory. To
      // figure this out, first normalize |path_| by stripping trailing
      // separators. Then compare the result to its DirName(). For the top-level
      // directory, e.g. "/" or "c:\\", the normalized path is equal to its own
      // DirName().
      base::FilePath stripped_path = path_.StripTrailingSeparators();
      if (stripped_path != stripped_path.DirName())
        pending_data_.append(net::GetParentDirectoryLink());
    }

    // Skip current / parent links from the listing.
    base::FilePath filename = data.info.GetName();
    if (filename.value() != base::FilePath::kCurrentDirectory &&
        filename.value() != base::FilePath::kParentDirectory) {
#if defined(OS_WIN)
      std::string raw_bytes;  // Empty on Windows means UTF-8 encoded name.
#elif defined(OS_POSIX)
      const std::string& raw_bytes = filename.value();
#endif
      pending_data_.append(net::GetDirectoryListingEntry(
          filename.LossyDisplayName(), raw_bytes, data.info.IsDirectory(),
          data.info.GetSize(), data.info.GetLastModifiedTime()));
    }

    MaybeTransferPendingData();
  }

  void OnListDone(int error) override {
    listing_result_ = error;
    lister_.reset();
    MaybeDeleteSelf();
  }

  void MaybeTransferPendingData() {
    if (transfer_in_progress_)
      return;

    transfer_in_progress_ = true;
    data_producer_->Write(pending_data_,
                          base::BindOnce(&FileURLDirectoryLoader::OnDataWritten,
                                         base::Unretained(this)));
    // The producer above will have already copied any parts of |pending_data_|
    // that couldn't be written immediately, so we can wipe it out here to begin
    // accumulating more data.
    pending_data_.clear();
  }

  void OnDataWritten(MojoResult result) {
    transfer_in_progress_ = false;

    int completion_status;
    if (result == MOJO_RESULT_OK) {
      if (!pending_data_.empty()) {
        // Keep flushing the data buffer as long as it's non-empty and pipe
        // writes are succeeding.
        MaybeTransferPendingData();
        return;
      }

      // If there's no pending data but the lister is still active, we simply
      // wait for more listing results.
      if (lister_)
        return;

      // At this point we know the listing is complete and all available data
      // has been transferred. We inherit the status of the listing operation.
      completion_status = listing_result_;
    } else {
      completion_status = net::ERR_FAILED;
    }

    client_->OnComplete(ResourceRequestCompletionStatus(completion_status));
    client_.reset();
    MaybeDeleteSelf();
  }

  base::FilePath path_;
  std::unique_ptr<net::DirectoryLister> lister_;
  bool wrote_header_ = false;
  int listing_result_;

  mojo::Binding<mojom::URLLoader> binding_;
  mojom::URLLoaderClientPtr client_;

  std::unique_ptr<mojo::StringDataPipeProducer> data_producer_;
  std::string pending_data_;
  bool transfer_in_progress_ = false;

  DISALLOW_COPY_AND_ASSIGN(FileURLDirectoryLoader);
};

}  // namespace

FileProtocolHandlerNetworkService::FileProtocolHandlerNetworkService(
    const base::FilePath& profile_path,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : profile_path_(profile_path), task_runner_(std::move(task_runner)) {}

FileProtocolHandlerNetworkService::~FileProtocolHandlerNetworkService() =
    default;

void FileProtocolHandlerNetworkService::CreateAndStartLoader(
    const ResourceRequest& request,
    mojom::URLLoaderRequest loader,
    mojom::URLLoaderClientPtr client) {
  base::FilePath file_path;
  const bool is_file = net::FileURLToFilePath(request.url, &file_path);
  if (is_file && file_path.EndsWithSeparator() && file_path.IsAbsolute()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&FileURLDirectoryLoader::CreateAndStart, profile_path_,
                       request, std::move(loader), client.PassInterface()));
  } else {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&FileURLLoader::CreateAndStart, profile_path_, request,
                       std::move(loader), client.PassInterface()));
  }
}

}  // namespace content
