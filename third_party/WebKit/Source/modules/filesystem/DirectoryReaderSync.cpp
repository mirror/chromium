/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "modules/filesystem/DirectoryReaderSync.h"

#include "bindings/core/v8/ExceptionState.h"
#include "modules/filesystem/DirectoryEntry.h"
#include "modules/filesystem/DirectoryEntrySync.h"
#include "modules/filesystem/EntrySync.h"
#include "modules/filesystem/FileEntrySync.h"
#include "modules/filesystem/FileSystemCallbacks.h"

namespace blink {

class DirectoryReaderSync::EntriesCallbackHelper final
    : public EntriesCallbacks::OnDidGetEntriesCallback {
 public:
  static EntriesCallbackHelper* Create(DirectoryReaderSync* directory_reader) {
    return new EntriesCallbackHelper(directory_reader);
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(directory_reader_);
    EntriesCallbacks::OnDidGetEntriesCallback::Trace(visitor);
  }

  void OnSuccess(EntryHeapVectorCarrier* entries) override {
    directory_reader_->entries_.ReserveCapacity(
        directory_reader_->entries_.size() + entries->Get().size());
    for (const auto& entry : entries->Get()) {
      directory_reader_->entries_.UncheckedAppend(
          EntrySync::Create(entry.Get()));
    }
  }

 private:
  explicit EntriesCallbackHelper(DirectoryReaderSync* directory_reader)
      : directory_reader_(directory_reader) {}

  Member<DirectoryReaderSync> directory_reader_;
};

DirectoryReaderSync::DirectoryReaderSync(DOMFileSystemBase* file_system,
                                         const String& full_path)
    : DirectoryReaderBase(file_system, full_path) {}

EntrySyncHeapVector DirectoryReaderSync::readEntries(
    ExceptionState& exception_state) {
  if (!callbacks_id_) {
    DCHECK(!sync_helper_);
    sync_helper_ = EntriesCallbacksSyncHelper::Create();
    callbacks_id_ = Filesystem()->ReadDirectory(
        this, full_path_,
        // We need to accumulate the entries in the directory, so
        // |sync_helper_->GetSuccessCallback()|, which is a simple getter,
        // doesn't fit for this purpose.
        EntriesCallbackHelper::Create(this), sync_helper_->GetErrorCallback(),
        DOMFileSystemBase::kSynchronous);
  }

  if (sync_helper_->GetErrorCode() == FileError::kOK && has_more_entries_ &&
      entries_.IsEmpty()) {
    CHECK(Filesystem()->WaitForAdditionalResult(callbacks_id_));
  }

  sync_helper_->GetResultOrThrow(exception_state);
  if (exception_state.HadException()) {
    return EntrySyncHeapVector();
  }

  EntrySyncHeapVector result;
  result.swap(entries_);
  return result;
}

void DirectoryReaderSync::Trace(blink::Visitor* visitor) {
  visitor->Trace(sync_helper_);
  visitor->Trace(entries_);
  DirectoryReaderBase::Trace(visitor);
}

}  // namespace blink
