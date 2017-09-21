// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_CLIENTS_MOJO_CDM_FILE_IO_H_
#define MEDIA_MOJO_CLIENTS_MOJO_CDM_FILE_IO_H_

#include <stdint.h>
#include <string>

#include "base/callback_forward.h"
#include "base/files/file.h"
#include "base/files/file_proxy.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "media/cdm/api/content_decryption_module.h"
#include "media/cdm/cdm_file_io.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace base {
class SequencedTaskRunner;
}

namespace cdm {
class FileIOClient;
}

namespace media {

// Implements a CdmFileIO that communicates with mojom::CdmStorage.
class MEDIA_MOJO_EXPORT MojoCdmFileIO : public CdmFileIO {
 public:
  MojoCdmFileIO(mojom::CdmStoragePtr storage, cdm::FileIOClient* client);
  ~MojoCdmFileIO() override;

  // CdmFileIO implementation.
  void Open(const char* file_name, uint32_t file_name_size) final;
  void Read() final;
  void Write(const uint8_t* data, uint32_t data_size) final;
  void Close() final;

 private:
  class FileInUseTracker;

  // Class to keep track of the current operation happening on the file. Once
  // the file is opened, only 1 read or write is allowed at a time.
  class StateManager {
   public:
    // Allowed states:
    //   kNone -> kOpening -> kOpened
    //   kOpened -> kReading -> kOpened
    //   kOpened -> kWriting -> kOpened
    enum class State { kNone, kOpening, kOpened, kReading, kWriting };

    StateManager() = default;
    ~StateManager() = default;

    // If possible, set the state to |op|. Returns true if the current state
    // allows the transition to |op|, false if not allowed.
    bool SetState(State op);

    State GetState() const { return current_state_; }

    // Returns true if a read or write operation is currently in progress,
    // false otherwise.
    bool IsReadOrWriteInProgress() {
      return current_state_ == State::kReading ||
             current_state_ == State::kWriting;
    }

   private:
    State current_state_ = State::kNone;
    DISALLOW_COPY_AND_ASSIGN(StateManager);
  };

  // Called when the CdmStorage::Open operation completes asynchronously.
  void OnOpenComplete(mojom::CdmStorage::Status status, base::File file);

  // Execute the Read() operation asynchronously.
  void OnGetInfoComplete(base::File::Error error, const base::File::Info& info);
  void OnFileRead(base::File::Error error, const char* data, int bytes_read);

  // Called when the Read() operation completes.
  void OnReadComplete(cdm::FileIOClient::Status status,
                      const uint8_t* data,
                      uint32_t data_size);

  // Execute the Write() operation asynchronously.
  void OnFileWritten(base::File::Error error, int bytes_written);
  void OnFileLengthSet(cdm::FileIOClient::Status status,
                       base::File::Error error);
  void OnFileFlushed(cdm::FileIOClient::Status result, base::File::Error error);

  // Called when the Write() operation completes.
  void OnWriteComplete(cdm::FileIOClient::Status status);

  // Helper function for asynchronous calls.
  void PostTask(base::OnceClosure task);

  mojom::CdmStoragePtr storage_;
  cdm::FileIOClient* client_;

  std::unique_ptr<FileInUseTracker> file_in_use_tracker_;

  // Keep track of the file being used. As this class can only be used for
  // accessing a single file, once |file_name_| is set it shouldn't be changed.
  std::string file_name_;

  // All access to the |file_| will be done as a sequential task so that
  // operations don't overlap.
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  base::FileProxy file_;

  // Keep track of operations in progress.
  StateManager state_manager_;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<MojoCdmFileIO> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoCdmFileIO);
};

}  // namespace media

#endif  // MEDIA_MOJO_CLIENTS_MOJO_CDM_FILE_IO_H_
