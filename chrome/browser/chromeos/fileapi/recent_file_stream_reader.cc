// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/recent_file_stream_reader.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/chromeos/fileapi/recent_context.h"
#include "chrome/browser/chromeos/fileapi/recent_file.h"
#include "chrome/browser/chromeos/fileapi/recent_model.h"
#include "chrome/browser/chromeos/fileapi/recent_util.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "storage/browser/fileapi/file_system_context.h"

using content::BrowserThread;

namespace chromeos {

RecentFileStreamReader::RecentFileStreamReader(
    const storage::FileSystemURL& recent_url,
    int64_t offset,
    int64_t max_bytes_to_read,
    const base::Time& expected_modification_time,
    storage::FileSystemContext* file_system_context,
    RecentModel* model)
    : offset_(offset),
      max_bytes_to_read_(max_bytes_to_read),
      expected_modification_time_(expected_modification_time),
      reader_resolved_(false),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ParseRecentUrlOnIOThread(
      file_system_context, recent_url,
      base::BindOnce(&RecentFileStreamReader::OnParseRecentUrl,
                     weak_ptr_factory_.GetWeakPtr(),
                     make_scoped_refptr(file_system_context),
                     // |model| is alive as long as a reference to
                     // |file_system_context| is kept.
                     base::Unretained(model)));
}

RecentFileStreamReader::~RecentFileStreamReader() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

int RecentFileStreamReader::Read(net::IOBuffer* buffer,
                                 int buffer_length,
                                 const net::CompletionCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!reader_resolved_) {
    pending_operations_.emplace_back(base::BindOnce(
        &RecentFileStreamReader::RunPendingRead, base::Unretained(this),
        make_scoped_refptr(buffer), buffer_length, callback));
    return net::ERR_IO_PENDING;
  }
  if (!underlying_reader_)
    return net::ERR_FILE_NOT_FOUND;
  return underlying_reader_->Read(buffer, buffer_length, callback);
}

int64_t RecentFileStreamReader::GetLength(
    const net::Int64CompletionCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!reader_resolved_) {
    pending_operations_.emplace_back(
        base::BindOnce(&RecentFileStreamReader::RunPendingGetLength,
                       base::Unretained(this), callback));
    return net::ERR_IO_PENDING;
  }
  if (!underlying_reader_)
    return net::ERR_FILE_NOT_FOUND;
  return underlying_reader_->GetLength(callback);
}

void RecentFileStreamReader::OnParseRecentUrl(
    scoped_refptr<storage::FileSystemContext> file_system_context,
    RecentModel* model,
    RecentContext context,
    const base::FilePath& path) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!context.is_valid()) {
    OnReaderResolved(nullptr);
    return;
  }

  model->GetFilesMap(context, false /* want_refresh */,
                     base::BindOnce(&RecentFileStreamReader::OnGetFilesMap,
                                    weak_ptr_factory_.GetWeakPtr(),
                                    file_system_context, path));
}

void RecentFileStreamReader::OnGetFilesMap(
    scoped_refptr<storage::FileSystemContext> file_system_context,
    const base::FilePath& path,
    const RecentModel::FilesMap& files_map) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  auto iter = files_map.find(path);
  if (iter == files_map.end()) {
    OnReaderResolved(nullptr);
    return;
  }

  RecentFile* file = iter->second.get();
  OnReaderResolved(file->CreateFileStreamReader(offset_, max_bytes_to_read_,
                                                expected_modification_time_,
                                                file_system_context.get()));
}

void RecentFileStreamReader::OnReaderResolved(
    std::unique_ptr<storage::FileStreamReader> underlying_reader) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!reader_resolved_);
  DCHECK(!underlying_reader_);

  underlying_reader_ = std::move(underlying_reader);
  reader_resolved_ = true;

  std::vector<base::OnceClosure> pending_operations;
  pending_operations.swap(pending_operations_);
  for (base::OnceClosure& callback : pending_operations) {
    std::move(callback).Run();
  }
}

void RecentFileStreamReader::RunPendingRead(
    scoped_refptr<net::IOBuffer> buffer,
    int buffer_length,
    const net::CompletionCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(reader_resolved_);
  int result = underlying_reader_ ? underlying_reader_->Read(
                                        buffer.get(), buffer_length, callback)
                                  : net::ERR_FILE_NOT_FOUND;
  if (result != net::ERR_IO_PENDING)
    callback.Run(result);
}

void RecentFileStreamReader::RunPendingGetLength(
    const net::Int64CompletionCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(reader_resolved_);
  int64_t result = underlying_reader_ ? underlying_reader_->GetLength(callback)
                                      : net::ERR_FILE_NOT_FOUND;
  if (result != net::ERR_IO_PENDING)
    callback.Run(result);
}

}  // namespace chromeos
