// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_CDM_FILE_IO_H_
#define MEDIA_MOJO_SERVICES_MOJO_CDM_FILE_IO_H_

#include <stdint.h>

#include <string>

#include "base/callback_forward.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "media/cdm/api/content_decryption_module.h"
#include "media/cdm/cdm_file_io.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"
#include "media/mojo/services/media_mojo_export.h"

namespace media {

// Implements a CdmFileIO that communicates with mojom::CdmStorage.
class MEDIA_MOJO_EXPORT MojoCdmFileIO : public CdmFileIO {
 public:
  using OpenCB =
      base::OnceCallback<void(const std::string& file_name,
                              mojom::CdmStorage::OpenCallback callback)>;

  MojoCdmFileIO(cdm::FileIOClient* client, OpenCB open_callback);
  ~MojoCdmFileIO() override;

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

  // Called by |open_callback_| when the file is opened.
  void OnOpenComplete(
      mojom::CdmStorage::Status status,
      base::File file,
      mojom::CdmFileReleaserAssociatedPtrInfo cdm_file_releaser);

  // Read() is run asynchronously.
  void DoReadAsync();
  void OnReadComplete(cdm::FileIOClient::Status status,
                      const uint8_t* data,
                      uint32_t data_size);

  // Write is done synchronously (to avoid having to copy the data), but
  // calling |client_| with the result is done asynchronously.
  void OnWriteCompleteAsync(cdm::FileIOClient::Status status);

  // Results of CdmFileIO operations are sent asynchronously via |client_|.
  cdm::FileIOClient* client_;

  // Used to open a file.
  OpenCB open_callback_;

  // Keep track of the file being used. As this class can only be used for
  // accessing a single file, once |file_name_| is set it shouldn't be changed.
  // |cdm_file_releaser_| is used to release the file when Close() is called.
  std::string file_name_;
  base::File file_;
  mojom::CdmFileReleaserAssociatedPtr cdm_file_releaser_;

  // Keep track of operations in progress.
  StateManager state_manager_;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<MojoCdmFileIO> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoCdmFileIO);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_CDM_FILE_IO_H_
