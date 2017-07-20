// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/pattern.h"
#include "content/browser/tracing/tracing_controller_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/tracing_controller.h"
#include "third_party/zlib/zlib.h"

namespace content {

namespace {

class StringTraceDataEndpoint : public TracingController::TraceDataEndpoint {
 public:
  typedef base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                              base::RefCountedString*)>
      CompletionCallback;

  explicit StringTraceDataEndpoint(CompletionCallback callback)
      : completion_callback_(callback) {}

  void ReceiveTraceFinalContents(
      std::unique_ptr<const base::DictionaryValue> metadata) override {
    std::string tmp = trace_.str();
    trace_.str("");
    trace_.clear();
    scoped_refptr<base::RefCountedString> str =
        base::RefCountedString::TakeString(&tmp);

    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(completion_callback_, base::Passed(std::move(metadata)),
                   base::RetainedRef(str)));
  }

  void ReceiveTraceChunk(std::unique_ptr<std::string> chunk) override {
    trace_ << *chunk;
  }

 private:
  ~StringTraceDataEndpoint() override {}

  CompletionCallback completion_callback_;
  std::ostringstream trace_;

  DISALLOW_COPY_AND_ASSIGN(StringTraceDataEndpoint);
};

class FileTraceDataEndpoint : public TracingController::TraceDataEndpoint {
 public:
  explicit FileTraceDataEndpoint(const base::FilePath& trace_file_path,
                                 const base::Closure& callback)
      : file_path_(trace_file_path),
        completion_callback_(callback),
        file_(NULL) {}

  void ReceiveTraceChunk(std::unique_ptr<std::string> chunk) override {
    BrowserThread::PostTask(
        BrowserThread::FILE, FROM_HERE,
        base::Bind(&FileTraceDataEndpoint::ReceiveTraceChunkOnFileThread, this,
                   base::Passed(std::move(chunk))));
  }

  void ReceiveTraceFinalContents(
      std::unique_ptr<const base::DictionaryValue>) override {
    BrowserThread::PostTask(
        BrowserThread::FILE, FROM_HERE,
        base::Bind(&FileTraceDataEndpoint::CloseOnFileThread, this));
  }

 private:
  ~FileTraceDataEndpoint() override { DCHECK(file_ == NULL); }

  void ReceiveTraceChunkOnFileThread(std::unique_ptr<std::string> chunk) {
    if (!OpenFileIfNeededOnFileThread())
      return;
    ignore_result(fwrite(chunk->c_str(), chunk->size(), 1, file_));
  }

  bool OpenFileIfNeededOnFileThread() {
    if (file_ != NULL)
      return true;
    file_ = base::OpenFile(file_path_, "w");
    if (file_ == NULL) {
      LOG(ERROR) << "Failed to open " << file_path_.value();
      return false;
    }
    return true;
  }

  void CloseOnFileThread() {
    if (OpenFileIfNeededOnFileThread()) {
      base::CloseFile(file_);
      file_ = NULL;
    }
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&FileTraceDataEndpoint::FinalizeOnUIThread, this));
  }

  void FinalizeOnUIThread() { completion_callback_.Run(); }

  base::FilePath file_path_;
  base::Closure completion_callback_;
  FILE* file_;

  DISALLOW_COPY_AND_ASSIGN(FileTraceDataEndpoint);
};

class CompressedTraceDataEndpoint
    : public TracingController::TraceDataEndpoint {
 public:
  explicit CompressedTraceDataEndpoint(
      scoped_refptr<TraceDataEndpoint> endpoint)
      : endpoint_(endpoint), already_tried_open_(false) {}

  void ReceiveTraceChunk(std::unique_ptr<std::string> chunk) override {
    BrowserThread::PostTask(
        BrowserThread::FILE, FROM_HERE,
        base::Bind(&CompressedTraceDataEndpoint::CompressOnFileThread, this,
                   base::Passed(std::move(chunk))));
  }

  void ReceiveTraceFinalContents(
      std::unique_ptr<const base::DictionaryValue> metadata) override {
    BrowserThread::PostTask(
        BrowserThread::FILE, FROM_HERE,
        base::Bind(&CompressedTraceDataEndpoint::CloseOnFileThread, this,
                   base::Passed(std::move(metadata))));
  }

 private:
  ~CompressedTraceDataEndpoint() override {}

  bool OpenZStreamOnFileThread() {
    DCHECK_CURRENTLY_ON(BrowserThread::FILE);
    if (stream_)
      return true;

    if (already_tried_open_)
      return false;

    already_tried_open_ = true;
    stream_.reset(new z_stream);
    *stream_ = {0};
    stream_->zalloc = Z_NULL;
    stream_->zfree = Z_NULL;
    stream_->opaque = Z_NULL;

    int result = deflateInit2(stream_.get(), Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                              // 16 is added to produce a gzip header + trailer.
                              MAX_WBITS + 16,
                              8,  // memLevel = 8 is default.
                              Z_DEFAULT_STRATEGY);
    return result == 0;
  }

  void CompressOnFileThread(std::unique_ptr<std::string> chunk) {
    DCHECK_CURRENTLY_ON(BrowserThread::FILE);
    if (!OpenZStreamOnFileThread())
      return;

    stream_->avail_in = chunk->size();
    stream_->next_in = reinterpret_cast<unsigned char*>(&*chunk->begin());
    DrainStreamOnFileThread(false);
  }

  void DrainStreamOnFileThread(bool finished) {
    DCHECK_CURRENTLY_ON(BrowserThread::FILE);

    int err;
    const int kChunkSize = 0x4000;
    char buffer[kChunkSize];
    do {
      stream_->avail_out = kChunkSize;
      stream_->next_out = (unsigned char*)buffer;
      err = deflate(stream_.get(), finished ? Z_FINISH : Z_NO_FLUSH);
      if (err != Z_OK && (!finished || err != Z_STREAM_END)) {
        LOG(ERROR) << "Deflate stream error: " << err;
        stream_.reset();
        return;
      }

      int bytes = kChunkSize - stream_->avail_out;
      if (bytes) {
        std::string compressed(buffer, bytes);
        endpoint_->ReceiveTraceChunk(base::MakeUnique<std::string>(compressed));
      }
    } while (stream_->avail_out == 0);
  }

  void CloseOnFileThread(
      std::unique_ptr<const base::DictionaryValue> metadata) {
    DCHECK_CURRENTLY_ON(BrowserThread::FILE);
    if (!OpenZStreamOnFileThread())
      return;

    DrainStreamOnFileThread(true);
    deflateEnd(stream_.get());
    stream_.reset();

    endpoint_->ReceiveTraceFinalContents(std::move(metadata));
  }

  scoped_refptr<TraceDataEndpoint> endpoint_;
  std::unique_ptr<z_stream> stream_;
  bool already_tried_open_;

  DISALLOW_COPY_AND_ASSIGN(CompressedTraceDataEndpoint);
};

}  // namespace

scoped_refptr<TracingController::TraceDataEndpoint>
TracingController::CreateStringEndpoint(
    const base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                              base::RefCountedString*)>& callback) {
  return new StringTraceDataEndpoint(callback);
}

scoped_refptr<TracingController::TraceDataEndpoint>
TracingController::CreateFileEndpoint(const base::FilePath& file_path,
                                      const base::Closure& callback) {
  return new FileTraceDataEndpoint(file_path, callback);
}

scoped_refptr<TracingController::TraceDataEndpoint>
TracingControllerImpl::CreateCompressedStringEndpoint(
    scoped_refptr<TraceDataEndpoint> endpoint) {
  return new CompressedTraceDataEndpoint(endpoint);
}

scoped_refptr<TracingController::TraceDataEndpoint>
TracingControllerImpl::CreateCallbackEndpoint(
    const base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                              base::RefCountedString*)>& callback) {
  return new StringTraceDataEndpoint(callback);
}

}  // namespace content
