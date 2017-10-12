// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_CDM_FILE_IO_H_
#define MEDIA_MOJO_SERVICES_MOJO_CDM_FILE_IO_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "media/cdm/api/content_decryption_module.h"
#include "media/cdm/cdm_file_io.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"
#include "media/mojo/services/media_mojo_export.h"

namespace media {

// TODO(crbug.com/774160): This class should do the Read/Write operations on a
// separate thread so as to not impact decoding happening on the same thread.
// The new thread may need to block shutdown so that the file is not corrupted.

// Implements a CdmFileIO that communicates with mojom::CdmStorage.
class MEDIA_MOJO_EXPORT MojoCdmFileIO : public CdmFileIO {
 public:
  MojoCdmFileIO(cdm::FileIOClient* client, mojom::CdmStorage* cdm_storage);
  ~MojoCdmFileIO() override;

  // CdmFileIO implementation.
  void Open(const char* file_name, uint32_t file_name_size) final;
  void Read() final;
  void Write(const uint8_t* data, uint32_t data_size) final;
  void Close() final;

 private:
  // Allowed states:
  //   kUnopened -> kOpening -> kOpened
  //   kUnopened -> kOpening -> kUnopened (if file in use)
  //   kUnopened -> kOpening -> kError (if file not available)
  //   kOpened -> kError (if error happens reading/writing the file)
  // All read/write operations are synchronous, so they can run as long as
  // state = kOpened.
  enum class State { kUnopened, kOpening, kOpened, kError };

  // Opening the file is done asynchronously. |file_name| is the name of the
  // file to be opened, and |can_open| is true if the file should be opened.
  void DoOpen(const std::string& file_name, bool can_open);

  // Called when the file is opened (or not).
  void OnFileOpened(mojom::CdmStorage::Status status,
                    base::File file,
                    mojom::CdmFileReleaserAssociatedPtrInfo cdm_file_releaser);

  // Reading the file is done asynchronously.
  void DoRead();

  // Write is done synchronously (to avoid having to copy the data), but
  // calling |client_| with the result must be done asynchronously.
  // PostWriteResult() is used to post the result to
  // NotifyClientOfWriteResult(), which reports the result to |client_|.
  void PostWriteResult(cdm::FileIOClient::Status status);
  void NotifyClientOfWriteResult(cdm::FileIOClient::Status status);

  // Results of CdmFileIO operations are sent asynchronously via |client_|.
  cdm::FileIOClient* client_;

  mojom::CdmStorage* cdm_storage_;

  // Keep track of the file being used. As this class can only be used for
  // accessing a single file, once |file_name_| is set it shouldn't be changed.
  base::FilePath file_name_;
  base::File file_;

  // |cdm_file_releaser_| is used to release the file when Close() is called.
  mojom::CdmFileReleaserAssociatedPtr cdm_file_releaser_;

  // Keep track of operations in progress.
  State state_ = State::kUnopened;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<MojoCdmFileIO> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoCdmFileIO);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_CDM_FILE_IO_H_
