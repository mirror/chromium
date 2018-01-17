// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_FILE_IMPL_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_FILE_IMPL_H_

#include "content/browser/download/download_file.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/browser/download/base_file.h"
#include "content/browser/download/download_source_stream.h"
#include "content/browser/download/rate_estimator.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_save_info.h"
#include "content/public/common/download_stream.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/simple_watcher.h"

namespace content {
class DownloadDestinationObserver;

class CONTENT_EXPORT DownloadFileImpl : public DownloadFile {
 public:
  // Takes ownership of the object pointed to by |save_info|.
  // |net_log| will be used for logging the download file's events.
  // May be constructed on any thread.  All methods besides the constructor
  // (including destruction) must occur in the same sequence.
  //
  // Note that the DownloadFileImpl automatically reads from the passed in
  // |stream|, and sends updates and status of those reads to the
  // DownloadDestinationObserver.
  DownloadFileImpl(std::unique_ptr<DownloadSaveInfo> save_info,
                   const base::FilePath& default_downloads_directory,
                   std::unique_ptr<DownloadManager::InputStream> stream,
                   uint32_t download_id,
                   base::WeakPtr<DownloadDestinationObserver> observer);

  ~DownloadFileImpl() override;

  // DownloadFile functions.
  void Initialize(const InitializeCallback& initialize_callback,
                  const CancelRequestCallback& cancel_request_callback,
                  const DownloadItem::ReceivedSlices& received_slices,
                  bool is_parallelizable) override;
  void AddInputStream(std::unique_ptr<DownloadManager::InputStream> stream,
                      int64_t offset,
                      int64_t length) override;
  void OnResponseCompleted(int64_t offset,
                           DownloadInterruptReason status) override;
  void RenameAndUniquify(const base::FilePath& full_path,
                         const RenameCompletionCallback& callback) override;
  void RenameAndAnnotate(const base::FilePath& full_path,
                         const std::string& client_guid,
                         const GURL& source_url,
                         const GURL& referrer_url,
                         const RenameCompletionCallback& callback) override;
  void Detach() override;
  void Cancel() override;
  void SetPotentialFileLength(int64_t length) override;
  const base::FilePath& FullPath() const override;
  bool InProgress() const override;
  void Pause() override;
  void Resume() override;

 protected:
  // For test class overrides.
  // Write data from the offset to the file.
  // On OS level, it will seek to the |offset| and write from there.
  virtual DownloadInterruptReason WriteDataToFile(int64_t offset,
                                                  const char* data,
                                                  size_t data_len);

  virtual base::TimeDelta GetRetryDelayForFailedRename(int attempt_number);

  virtual bool ShouldRetryFailedRename(DownloadInterruptReason reason);

  virtual DownloadInterruptReason HandleStreamCompletionStatus(
      DownloadSourceStream* source_stream);

 private:
  friend class DownloadFileTest;

  DownloadFileImpl(std::unique_ptr<DownloadSaveInfo> save_info,
                   const base::FilePath& default_downloads_directory,
                   uint32_t download_id,
                   base::WeakPtr<DownloadDestinationObserver> observer);

  // Options for RenameWithRetryInternal.
  enum RenameOption {
    UNIQUIFY = 1 << 0,  // If there's already a file on disk that conflicts with
                        // |new_path|, try to create a unique file by appending
                        // a uniquifier.
    ANNOTATE_WITH_SOURCE_INFORMATION = 1 << 1
  };

  struct RenameParameters {
    RenameParameters(RenameOption option,
                     const base::FilePath& new_path,
                     const RenameCompletionCallback& completion_callback);
    ~RenameParameters();

    RenameOption option;
    base::FilePath new_path;
    std::string client_guid;  // See BaseFile::AnnotateWithSourceInformation()
    GURL source_url;          // See BaseFile::AnnotateWithSourceInformation()
    GURL referrer_url;        // See BaseFile::AnnotateWithSourceInformation()
    int retries_left;         // RenameWithRetryInternal() will
                              // automatically retry until this
                              // count reaches 0. Each attempt
                              // decrements this counter.
    base::TimeTicks time_of_first_failure;  // Set to empty at first, but is set
                                            // when a failure is first
                                            // encountered. Used for UMA.
    RenameCompletionCallback completion_callback;
  };

  // Rename file_ based on |parameters|.
  void RenameWithRetryInternal(std::unique_ptr<RenameParameters> parameters);

  // Send an update on our progress.
  void SendUpdate();

  // Called before the data is written to disk.
  void WillWriteToDisk(size_t data_len);

  // For a given DownloadSourceStream object and the bytes available to write,
  // determine the actual number of bytes it can write to the disk. For parallel
  // downloading, if the first disk IO writes to a location that is already
  // written by another stream, the current stream should stop writing. Returns
  // true if the stream can write no more data and should be finished, returns
  // false otherwise.
  bool CalculateBytesToWrite(DownloadSourceStream* source_stream,
                             size_t bytes_available_to_write,
                             size_t* bytes_to_write);

  // Called when a new DownloadSourceStream object is added.
  void OnSourceStreamAdded(DownloadSourceStream* source_stream);

  // Called when there's some activity on the input data that needs to be
  // handled.
  void StreamActive(DownloadSourceStream* source_stream, MojoResult result);

  // Register callback and start to read data from the stream.
  void RegisterAndActivateStream(DownloadSourceStream* source_stream);

  // Called when a stream completes.
  void OnStreamCompleted(DownloadSourceStream* source_stream);

  // Notify |observer_| about the download status.
  void NotifyObserver(DownloadSourceStream* source_stream,
                      DownloadInterruptReason reason,
                      DownloadSourceStream::StreamState stream_state,
                      bool should_terminate);

  // Adds a new slice to |received_slices_| and update the existing entries in
  // |source_streams_| as their lengths will change.
  // TODO(qinmin): add a test for this function.
  void AddNewSlice(int64_t offset, int64_t length);

  // Check if download is completed.
  bool IsDownloadCompleted();

  // Return the total valid bytes received in the target file.
  // If the file is a sparse file, return the total number of valid bytes.
  // Otherwise, return the current file size.
  int64_t TotalBytesReceived() const;

  // Helper method to handle stream error
  void HandleStreamError(DownloadSourceStream* source_stream,
                         DownloadInterruptReason reason);

  // Check whether this file is potentially sparse.
  bool IsSparseFile() const;

  // Given a DownloadSourceStream object, returns its neighbor that preceds it
  // if SourceStreams are ordered by their offsets
  DownloadSourceStream* FindPrecedingNeighbor(
      DownloadSourceStream* source_stream);

  // See |cancel_request_callback_|.
  void CancelRequest(int64_t offset);

  // Print the internal states for debugging.
  void DebugStates() const;

  // The base file instance.
  BaseFile file_;

  // DownloadSaveInfo provided during construction. Since the DownloadFileImpl
  // can be created on any thread, this holds the save_info_ until it can be
  // used to initialize file_ on the download sequence.
  std::unique_ptr<DownloadSaveInfo> save_info_;

  // The default directory for creating the download file.
  base::FilePath default_download_directory_;

  // Map of the offset and the source stream that represents the slice
  // starting from offset.
  typedef std::unordered_map<int64_t, std::unique_ptr<DownloadSourceStream>>
      SourceStreams;
  SourceStreams source_streams_;

  // Used to cancel the request on UI thread, since the DownloadSourceStream
  //  can't close the underlying resource writing to the pipe.
  CancelRequestCallback cancel_request_callback_;

  // Used to trigger progress updates.
  std::unique_ptr<base::RepeatingTimer> update_timer_;

  // Potential file length. A range request with an offset larger than this
  // value will fail. So the actual file length cannot be larger than this.
  int64_t potential_file_length_;

  // Statistics
  size_t bytes_seen_;
  base::TimeDelta disk_writes_time_;
  base::TimeTicks download_start_;
  RateEstimator rate_estimator_;
  int num_active_streams_;
  bool record_stream_bandwidth_;
  base::TimeTicks last_update_time_;
  size_t bytes_seen_with_parallel_streams_;
  size_t bytes_seen_without_parallel_streams_;
  base::TimeDelta download_time_with_parallel_streams_;
  base::TimeDelta download_time_without_parallel_streams_;

  std::vector<DownloadItem::ReceivedSlice> received_slices_;

  // Used to track whether the download is paused or not. This value is ignored
  // when network service is disabled as download pause/resumption is handled
  // by DownloadRequestCore in that case.
  bool is_paused_;

  uint32_t download_id_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtr<DownloadDestinationObserver> observer_;
  base::WeakPtrFactory<DownloadFileImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DownloadFileImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_FILE_IMPL_H_
