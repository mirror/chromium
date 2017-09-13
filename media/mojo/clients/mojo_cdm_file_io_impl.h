// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_CLIENTS_MOJO_CDM_FILE_IO_IMPL_H_
#define MEDIA_MOJO_CLIENTS_MOJO_CDM_FILE_IO_IMPL_H_

#include <stdint.h>
#include <string>

#include "base/callback_forward.h"
#include "base/files/file.h"
#include "base/files/file_proxy.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "media/cdm/api/content_decryption_module.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"

namespace base {
class SequencedTaskRunner;
}

namespace cdm {
class FileIOClient;
}

namespace media {

// Implements a version of cdm::FileIO that communicates with mojom::CdmStorage.
class MojoCdmFileIOImpl : public cdm::FileIO {
 public:
  MojoCdmFileIOImpl(const std::string& key_system,
                    media::mojom::CdmStoragePtr storage,
                    cdm::FileIOClient* client);

  // CdmFileIO implementation.
  void Open(const char* file_name, uint32_t file_name_size) final;
  void Read() final;
  void Write(const uint8_t* data, uint32_t data_size) final;
  void Close() final;

 private:
  // Class to keep track of the current operation happening on the file. Once
  // the file is opened, only 1 read or write is allowed at a time.
  class StateManager {
   public:
    enum class OperationType {
      kNone,
      kOpening,
      kOpened,
      kRead,
      kWrite,
      kClosed
    };

    StateManager() = default;
    ~StateManager() = default;

    // If possible, set the state to |op|. Returns true if the current state
    // allows the transition to |op|, false if not allowed.
    bool SetState(OperationType op);

    OperationType GetState() { return current_state_; }

    // Returns true if a read or write operation is currently in progress,
    // false otherwise.
    bool IsReadOrWriteInProgress() {
      return current_state_ == OperationType::kRead ||
             current_state_ == OperationType::kWrite;
    }

   private:
    OperationType current_state_ = OperationType::kNone;
    DISALLOW_COPY_AND_ASSIGN(StateManager);
  };

  // Always use Close() to release |this| object.
  ~MojoCdmFileIOImpl() override;

  // Called when the CdmStorage::Initialize operation completes asynchronously.
  void OnInitializeComplete(bool status);

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

  // Called when the Close() operation completes.
  void OnCloseComplete(bool status);

  // Helper function for asynchronous calls.
  void PostTask(base::OnceClosure task);

  std::string key_system_;
  media::mojom::CdmStoragePtr storage_;
  cdm::FileIOClient* client_;

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
  base::WeakPtrFactory<MojoCdmFileIOImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoCdmFileIOImpl);
};

}  // namespace media

#endif  // MEDIA_MOJO_CLIENTS_MOJO_CDM_FILE_IO_IMPL_H_
