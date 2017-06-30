// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_registry_impl.h"

#include "base/barrier_closure.h"
#include "base/callback_helpers.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_impl.h"
#include "storage/browser/blob/blob_storage_context.h"

namespace storage {

namespace {

using MemoryStrategy = BlobMemoryController::Strategy;

bool CalculateBlobMemorySize(const std::vector<mojom::DataElementPtr>& elements,
                             size_t* shortcut_bytes,
                             uint64_t* total_bytes) {
  DCHECK(shortcut_bytes);
  DCHECK(total_bytes);

  base::CheckedNumeric<uint64_t> total_size_checked = 0;
  base::CheckedNumeric<size_t> shortcut_size_checked = 0;
  for (const auto& e : elements) {
    if (e->is_bytes()) {
      const auto& bytes = e->get_bytes();
      total_size_checked += bytes->length;
      if (bytes->embedded_data) {
        if (bytes->embedded_data->size() != bytes->length)
          return false;
        shortcut_size_checked += bytes->length;
      }
    } else {
      continue;
    }
    if (!total_size_checked.IsValid() || !shortcut_size_checked.IsValid())
      return false;
  }
  *shortcut_bytes = shortcut_size_checked.ValueOrDie();
  *total_bytes = total_size_checked.ValueOrDie();
  return true;
}

}  // namespace

class BlobRegistryImpl::BlobUnderConstruction {
 public:
  BlobUnderConstruction(BlobRegistryImpl* blob_registry,
                        const std::string& uuid,
                        const std::string& content_type,
                        const std::string& content_disposition,
                        std::vector<mojom::DataElementPtr> elements,
                        mojo::ReportBadMessageCallback bad_message_callback)
      : blob_registry_(blob_registry),
        builder_(uuid),
        elements_(std::move(elements)),
        bad_message_callback_(std::move(bad_message_callback)),
        data_pipe_watcher_(FROM_HERE,
                           mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC),
        weak_ptr_factory_(this) {
    builder_.set_content_type(content_type);
    builder_.set_content_disposition(content_disposition);
  }

  // Call this after constructing to kick of fetching of UUIDs of blobs
  // referenced by this new blob. This (and any further methods) could end up
  // deleting |this| by removing it from the blobs_under_construction_
  // collection in the blob service.
  void StartTransportation();

  ~BlobUnderConstruction() {}

  const std::string& uuid() const { return builder_.uuid(); }

 private:
  BlobStorageContext* context() const { return blob_registry_->context_; }

  // Marks this blob as broken. If an optional |bad_message_reason| is provided,
  // this will also report a BadMessage on the binding over which the initial
  // Register request was received.
  // Also deletes |this| by removing it from the blobs_under_construction_ list.
  void MarkAsBroken(BlobStatus reason,
                    const std::string& bad_message_reason = "") {
    DCHECK(BlobStatusIsError(reason));
    DCHECK_EQ(bad_message_reason.empty(), !BlobStatusIsBadIPC(reason));
    // The blob might have been dereferenced already, in which case it may no
    // longer exist. If that happens just skip calling cancel.
    if (context()->registry().HasEntry(uuid()))
      context()->CancelBuildingBlob(uuid(), reason);
    if (!bad_message_reason.empty())
      std::move(bad_message_callback_).Run(bad_message_reason);
    MarkAsFinishedAndDeleteSelf();
  }

  void MarkAsFinishedAndDeleteSelf() {
    blob_registry_->blobs_under_construction_.erase(uuid());
  }

  // Called when the UUID of a referenced blob is received.
  void ReceivedBlobUUID(size_t blob_index, const std::string& uuid);

  // Called by either StartFetchingBlobUUIDs or ReceivedBlobUUID when all the
  // UUIDs of referenced blobs have been resolved. Starts checking for circular
  // references. Before we can proceed with actually building the blob, all
  // referenced blobs also need to have resolved their referenced blobs (to
  // always be able to calculate the size of the newly built blob). To ensure
  // this we might have to wait for one or more possibly indirectly dependent
  // blobs to also have resolved the UUIDs of their dependencies. This waiting
  // is kicked of by this method.
  void ResolvedAllBlobUUIDs();

  void DependentBlobReady(BlobStatus status);

  // Called when all blob dependencies have been resolved, and we're sure there
  // are no circular dependencies. This finally kicks of the actually building
  // of the blob, and figures out how to transport any bytes that might need
  // transporting.
  void ResolvedAllBlobDependencies();

  // Called when memory has been reserved for this blob and transport can begin.
  void OnReadyForTransport(
      BlobStatus status,
      std::vector<BlobMemoryController::FileCreationInfo> file_infos);

  void SendReplyRequests();
  void OnBytesReply(size_t element_index, const std::vector<uint8_t>& data);

  void SendDataPipeRequests(size_t next_element);
  void OnDataPipeReadable(size_t element_index, MojoResult result);

  void SendFileRequests(
      std::vector<BlobMemoryController::FileCreationInfo> file_infos);
  void OnFileReply(size_t builder_element_index,
                   const scoped_refptr<ShareableFileReference>& file_reference,
                   base::Optional<base::Time> time_file_modified);

  // Called when all data has been transported.
  void CompleteTransport();

#if DCHECK_IS_ON()
  // Returns true if the DAG made up by this blob and any other blobs that
  // are currently being built by BlobRegistryImpl contains any cycles.
  // |path_from_root| should contain all the nodes that have been visited so
  // far on a path from whatever node we started our search from.
  bool ContainsCycles(
      std::unordered_set<BlobUnderConstruction*>* path_from_root);
#endif

  // BlobRegistryImpl we belong to.
  BlobRegistryImpl* blob_registry_;

  // BlobDataBuilder for the blob under construction. Is created in the
  // constructor, but not filled until all referenced blob UUIDs have been
  // resolved.
  BlobDataBuilder builder_;

  // Elements as passed in to Register.
  std::vector<mojom::DataElementPtr> elements_;

  // Callback to report a BadMessage on the binding on which Register was
  // called.
  mojo::ReportBadMessageCallback bad_message_callback_;

  // Transport strategy to use when transporting data.
  MemoryStrategy memory_strategy_ = MemoryStrategy::NONE_NEEDED;

  // List of UUIDs for referenced blobs. Same size as |elements_|. All entries
  // for non-blob elements will remain empty strings.
  std::vector<std::string> referenced_blob_uuids_;

  // Number of blob UUIDs that have been resolved.
  size_t resolved_blob_uuid_count_ = 0;

  // Number of dependent blobs that have started constructing.
  size_t ready_dependent_blob_count_ = 0;

  struct FutureFileData {
    mojom::BytesProvider* provider;
    uint64_t source_offset;
    uint64_t source_size;
    uint64_t file_offset;
    size_t file_idx;
    size_t builder_element_index;
  };
  std::vector<FutureFileData> future_file_data_;

  size_t num_unresolved_bytes_requests_ = 0;

  mojo::ScopedDataPipeConsumerHandle data_pipe_handle_;
  mojo::SimpleWatcher data_pipe_watcher_;
  size_t current_data_pipe_offset_ = 0;

  base::WeakPtrFactory<BlobUnderConstruction> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(BlobUnderConstruction);
};

void BlobRegistryImpl::BlobUnderConstruction::StartTransportation() {
  size_t blob_count = 0;
  for (size_t i = 0; i < elements_.size(); ++i) {
    const auto& element = elements_[i];
    if (element->is_blob()) {
      if (element->get_blob()->blob.encountered_error()) {
        // Will delete |this|.
        MarkAsBroken(BlobStatus::ERR_REFERENCED_BLOB_BROKEN);
        return;
      }

      // If connection to blob is broken, something bad happened, so mark this
      // new blob as broken, which will delete |this| and keep it from doing
      // unneeded extra work.
      element->get_blob()->blob.set_connection_error_handler(base::BindOnce(
          &BlobUnderConstruction::MarkAsBroken, weak_ptr_factory_.GetWeakPtr(),
          BlobStatus::ERR_REFERENCED_BLOB_BROKEN, ""));

      element->get_blob()->blob->GetInternalUUID(
          base::BindOnce(&BlobUnderConstruction::ReceivedBlobUUID,
                         weak_ptr_factory_.GetWeakPtr(), blob_count++));
    } else if (element->is_bytes()) {
      if (element->get_bytes()->data.encountered_error()) {
        // Will delete |this|.
        MarkAsBroken(BlobStatus::ERR_SOURCE_DIED_IN_TRANSIT);
        return;
      }

      element->get_bytes()->data.set_connection_error_handler(base::BindOnce(
          &BlobUnderConstruction::MarkAsBroken, weak_ptr_factory_.GetWeakPtr(),
          BlobStatus::ERR_SOURCE_DIED_IN_TRANSIT, ""));
    }
  }
  referenced_blob_uuids_.resize(blob_count);

  // TODO(mek): Do we need some kind of timeout for fetching the UUIDs?
  // Without it a blob could forever remaing pending if a renderer sends us
  // a BlobPtr connected to a (malicious) non-responding implementation.

  // Do some basic validation of bytes to transport, and determine memory
  // transport strategy to use later.
  uint64_t transport_memory_size = 0;
  size_t shortcut_size = 0;
  if (!CalculateBlobMemorySize(elements_, &shortcut_size,
                               &transport_memory_size)) {
    MarkAsBroken(BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS,
                 "Invalid byte element sizes in BlobRegistry::Register");
    return;
  }

  const BlobMemoryController& memory_controller =
      context()->memory_controller();
  memory_strategy_ =
      memory_controller.DetermineStrategy(shortcut_size, transport_memory_size);
  if (memory_strategy_ == MemoryStrategy::TOO_LARGE) {
    MarkAsBroken(BlobStatus::ERR_OUT_OF_MEMORY);
    return;
  }

  // If there were no unresolved blobs, immediately proceed to the next step.
  // Currently this will only happen if there are no blobs referenced
  // whatsoever, but hopefully in the future blob UUIDs will be cached in the
  // message pipe handle, making things much more efficient in the common case.
  if (resolved_blob_uuid_count_ == referenced_blob_uuids_.size())
    ResolvedAllBlobUUIDs();
}

void BlobRegistryImpl::BlobUnderConstruction::ReceivedBlobUUID(
    size_t blob_index,
    const std::string& uuid) {
  DCHECK(referenced_blob_uuids_[blob_index].empty());
  DCHECK_LT(resolved_blob_uuid_count_, referenced_blob_uuids_.size());

  referenced_blob_uuids_[blob_index] = uuid;
  if (++resolved_blob_uuid_count_ == referenced_blob_uuids_.size())
    ResolvedAllBlobUUIDs();
}

void BlobRegistryImpl::BlobUnderConstruction::ResolvedAllBlobUUIDs() {
  DCHECK_EQ(resolved_blob_uuid_count_, referenced_blob_uuids_.size());

#if DCHECK_IS_ON()
  std::unordered_set<BlobUnderConstruction*> visited;
  if (ContainsCycles(&visited)) {
    MarkAsBroken(BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS,
                 "Cycles in blob references in BlobRegistry::Register");
    return;
  }
#endif

  for (const std::string& blob_uuid : referenced_blob_uuids_) {
    if (blob_uuid.empty() || blob_uuid == uuid() ||
        !context()->registry().HasEntry(blob_uuid)) {
      // Will delete |this|.
      MarkAsBroken(BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS,
                   "Bad blob references in BlobRegistry::Register");
      return;
    }

    std::unique_ptr<BlobDataHandle> handle =
        context()->GetBlobDataFromUUID(blob_uuid);
    handle->RunOnConstructionBegin(
        base::Bind(&BlobUnderConstruction::DependentBlobReady,
                   weak_ptr_factory_.GetWeakPtr()));
  }

  if (referenced_blob_uuids_.size() == 0)
    ResolvedAllBlobDependencies();
}

void BlobRegistryImpl::BlobUnderConstruction::DependentBlobReady(
    BlobStatus status) {
  if (BlobStatusIsError(status)) {
    // Will delete |this|.
    MarkAsBroken(BlobStatus::ERR_REFERENCED_BLOB_BROKEN);
    return;
  }
  if (++ready_dependent_blob_count_ == referenced_blob_uuids_.size())
    ResolvedAllBlobDependencies();
}

void BlobRegistryImpl::BlobUnderConstruction::ResolvedAllBlobDependencies() {
  DCHECK_EQ(resolved_blob_uuid_count_, referenced_blob_uuids_.size());
  DCHECK_EQ(ready_dependent_blob_count_, referenced_blob_uuids_.size());

  const auto& limits = context()->memory_controller().limits();

  auto blob_uuid_it = referenced_blob_uuids_.begin();
  size_t file_idx = 0;
  size_t i = 0;
  uint64_t current_file_size = 0;
  for (const auto& element : elements_) {
    if (element->is_bytes() &&
        memory_strategy_ == MemoryStrategy::NONE_NEEDED) {
      const auto& bytes = element->get_bytes();
      DCHECK(bytes->embedded_data);
      builder_.AppendData(
          reinterpret_cast<const char*>(bytes->embedded_data->data()),
          bytes->length);
    } else if (element->is_bytes()) {
      if (memory_strategy_ == MemoryStrategy::FILE) {
        uint64_t source_offset = 0;
        while (source_offset < element->get_bytes()->length) {
          uint64_t block_size =
              std::min(element->get_bytes()->length - source_offset,
                       limits.max_file_size - current_file_size);
          size_t block_idx = builder_.AppendFutureFile(current_file_size,
                                                       block_size, file_idx);

          FutureFileData d;
          d.provider = element->get_bytes()->data.get();
          d.source_offset = source_offset;
          d.source_size = block_size;
          d.file_offset = current_file_size;
          d.file_idx = file_idx;
          d.builder_element_index = block_idx;
          future_file_data_.push_back(std::move(d));
          num_unresolved_bytes_requests_++;

          source_offset += block_size;
          current_file_size += block_size;
          if (current_file_size >= limits.max_file_size) {
            current_file_size = 0;
            file_idx++;
          }
        }
      } else {
        // TODO(mek): Mojo datapipes take care of chunking of data at the
        // transport level, but we might still want to chunk up future data here
        // as well to improve memory sharing between sliced blobs. Or
        // alternatively that logic could be build into BlobDataBuilder itself.
        size_t block_idx =
            builder_.AppendFutureData(element->get_bytes()->length);
        DCHECK_EQ(block_idx, i);
        num_unresolved_bytes_requests_++;
      }
    } else if (element->is_file()) {
      const auto& f = element->get_file();
      builder_.AppendFile(f->path, f->offset, f->length,
                          f->expected_modification_time.value_or(base::Time()));
    } else if (element->is_file_filesystem()) {
      const auto& f = element->get_file_filesystem();
      builder_.AppendFileSystemFile(
          f->url, f->offset, f->length,
          f->expected_modification_time.value_or(base::Time()));
    } else if (element->is_blob()) {
      DCHECK(blob_uuid_it != referenced_blob_uuids_.end());
      const std::string& blob_uuid = *blob_uuid_it++;
      builder_.AppendBlob(blob_uuid, element->get_blob()->offset,
                          element->get_blob()->length);
    }
    ++i;
  }
  // OnReadyForTransport can be called synchronously, which can call
  // MarkAsFinishedAndDeleteSelf synchronously, so don't access any members
  // after this call.
  std::unique_ptr<BlobDataHandle> new_handle =
      context()->BuildPreregisteredBlob(
          builder_, base::Bind(&BlobUnderConstruction::OnReadyForTransport,
                               weak_ptr_factory_.GetWeakPtr()));

  // TODO(mek): Update BlobImpl with new BlobDataHandle. Although handles
  // only differ in their size() attribute, which is currently not used by
  // BlobImpl.
}

void BlobRegistryImpl::BlobUnderConstruction::OnReadyForTransport(
    BlobStatus status,
    std::vector<BlobMemoryController::FileCreationInfo> file_infos) {
  if (!BlobStatusIsPending(status)) {
    // Done or error.
    MarkAsFinishedAndDeleteSelf();
    return;
  }
  switch (memory_strategy_) {
    case MemoryStrategy::IPC:
      SendReplyRequests();
      break;
    case MemoryStrategy::SHARED_MEMORY:
      SendDataPipeRequests(0);
      break;
    case MemoryStrategy::FILE:
      SendFileRequests(std::move(file_infos));
      break;
    case MemoryStrategy::NONE_NEEDED:
      CompleteTransport();
      return;
    default:
      NOTREACHED();
  }
}

void BlobRegistryImpl::BlobUnderConstruction::SendReplyRequests() {
  for (size_t i = 0; i < elements_.size(); ++i) {
    const auto& element = elements_[i];
    if (!element->is_bytes())
      continue;

    element->get_bytes()->data->RequestAsReply(
        base::BindOnce(&BlobUnderConstruction::OnBytesReply,
                       weak_ptr_factory_.GetWeakPtr(), i));
  }
}

void BlobRegistryImpl::BlobUnderConstruction::OnBytesReply(
    size_t element_index,
    const std::vector<uint8_t>& data) {
  const auto& element = elements_[element_index];
  DCHECK(element->is_bytes());
  if (data.size() != element->get_bytes()->length) {
    DVLOG(1) << "Invalid data size " << data.size() << " vs requested size of "
             << element->get_bytes()->length;
    MarkAsBroken(BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS,
                 "Invalid data size in reply to BytesProvider::RequestAsReply");
    return;
  }
  if (!builder_.PopulateFutureData(element_index,
                                   reinterpret_cast<const char*>(data.data()),
                                   0, data.size())) {
    MarkAsBroken(
        BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS,
        "Error populating data in reply to BytesProvider::RequestAsReply");
    return;
  }

  // No longer need BytesProvider for this element, so clear it out to avoid
  // errors from breaking the blob.
  element->get_bytes()->data.reset();

  num_unresolved_bytes_requests_--;
  if (num_unresolved_bytes_requests_ == 0)
    CompleteTransport();
}

void BlobRegistryImpl::BlobUnderConstruction::SendDataPipeRequests(
    size_t next_element) {
  while (next_element < elements_.size() &&
         !elements_[next_element]->is_bytes())
    next_element++;
  if (next_element >= elements_.size()) {
    CompleteTransport();
    return;
  }

  DCHECK(!data_pipe_handle_.is_valid());
  mojo::ScopedDataPipeProducerHandle producer_handle;
  MojoCreateDataPipeOptions options;
  options.struct_size = sizeof(MojoCreateDataPipeOptions);
  options.flags = MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE;
  options.element_num_bytes = 1;
  options.capacity_num_bytes =
      context()->memory_controller().limits().max_shared_memory_size;
  MojoResult result =
      CreateDataPipe(&options, &producer_handle, &data_pipe_handle_);
  if (result != MOJO_RESULT_OK) {
    DVLOG(1) << "Unable to create data pipe for blob transfer.";
    MarkAsBroken(BlobStatus::ERR_OUT_OF_MEMORY);
    return;
  }

  current_data_pipe_offset_ = 0;

  const auto& element = elements_[next_element];
  element->get_bytes()->data->RequestAsStream(std::move(producer_handle));
  data_pipe_watcher_.Watch(
      data_pipe_handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
      base::Bind(&BlobUnderConstruction::OnDataPipeReadable,
                 weak_ptr_factory_.GetWeakPtr(), next_element));

  // No longer need BytesProvider for this element, so clear it out to avoid
  // errors from breaking the blob.
  element->get_bytes()->data.reset();
}

void BlobRegistryImpl::BlobUnderConstruction::OnDataPipeReadable(
    size_t element_index,
    MojoResult result) {
  if (result == MOJO_RESULT_CANCELLED ||
      result == MOJO_RESULT_FAILED_PRECONDITION) {
    // Data pipe broke before we received all the data.
    MarkAsBroken(BlobStatus::ERR_SOURCE_DIED_IN_TRANSIT);
    return;
  }
  DCHECK_EQ(result, MOJO_RESULT_OK);

  // TODO(mek): Use two-phase reads to directly copy from data pipe to
  // BlobDataBuilder.
  while (true) {
    uint32_t num_bytes = 0;
    MojoResult read_result =
        mojo::ReadDataRaw(data_pipe_handle_.get(), nullptr, &num_bytes,
                          MOJO_READ_DATA_FLAG_QUERY);
    if (read_result == MOJO_RESULT_SHOULD_WAIT)
      break;
    DCHECK_EQ(read_result, MOJO_RESULT_OK);

    std::vector<char> data(num_bytes);
    read_result =
        mojo::ReadDataRaw(data_pipe_handle_.get(), data.data(), &num_bytes,
                          MOJO_READ_DATA_FLAG_ALL_OR_NONE);
    if (read_result == MOJO_RESULT_SHOULD_WAIT)
      break;
    DCHECK_EQ(read_result, MOJO_RESULT_OK);

    if (!builder_.PopulateFutureData(element_index, data.data(),
                                     current_data_pipe_offset_, num_bytes)) {
      MarkAsBroken(
          BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS,
          "Error populating data in reply to BytesProvider::RequestAsStream");
      return;
    }
    current_data_pipe_offset_ += num_bytes;

    if (current_data_pipe_offset_ >=
        elements_[element_index]->get_bytes()->length) {
      // Done with this stream, on to the next.
      // TODO(mek): Should this wait to see if more data than expected gets
      // written?
      data_pipe_watcher_.Cancel();
      data_pipe_handle_.reset();
      SendDataPipeRequests(element_index + 1);
      return;
    }
  }
}

void BlobRegistryImpl::BlobUnderConstruction::SendFileRequests(
    std::vector<BlobMemoryController::FileCreationInfo> file_infos) {
  DCHECK_EQ(file_infos.size(), future_file_data_.back().file_idx + 1);
  for (const auto& block : future_file_data_) {
    block.provider->RequestAsFile(
        block.source_offset, block.source_size,
        file_infos[block.file_idx].file.Duplicate(), block.file_offset,
        base::BindOnce(&BlobUnderConstruction::OnFileReply,
                       weak_ptr_factory_.GetWeakPtr(),
                       block.builder_element_index,
                       file_infos[block.file_idx].file_reference));
  }
}

void BlobRegistryImpl::BlobUnderConstruction::OnFileReply(
    size_t builder_element_index,
    const scoped_refptr<ShareableFileReference>& file_reference,
    base::Optional<base::Time> time_file_modified) {
  if (!builder_.PopulateFutureFile(builder_element_index, file_reference,
                                   time_file_modified.value_or(base::Time()))) {
    MarkAsBroken(
        BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS,
        "Error populating data in reply to BytesProvider::RequestAsFile");
    return;
  }

  num_unresolved_bytes_requests_--;
  if (num_unresolved_bytes_requests_ == 0)
    CompleteTransport();
}

void BlobRegistryImpl::BlobUnderConstruction::CompleteTransport() {
  // The blob might have been dereferenced already, in which case it may no
  // longer exist. If that happens just skip calling Complete.
  // TODO(mek): Stop building sooner if a blob is no longer referenced.
  if (context()->registry().HasEntry(uuid()))
    context()->NotifyTransportComplete(uuid());
  MarkAsFinishedAndDeleteSelf();
}

#if DCHECK_IS_ON()
bool BlobRegistryImpl::BlobUnderConstruction::ContainsCycles(
    std::unordered_set<BlobUnderConstruction*>* path_from_root) {
  if (!path_from_root->insert(this).second)
    return true;
  for (const std::string& blob_uuid : referenced_blob_uuids_) {
    if (blob_uuid.empty())
      continue;
    auto it = blob_registry_->blobs_under_construction_.find(blob_uuid);
    if (it == blob_registry_->blobs_under_construction_.end())
      continue;
    if (it->second->ContainsCycles(path_from_root))
      return true;
  }
  path_from_root->erase(this);
  return false;
}
#endif

BlobRegistryImpl::BlobRegistryImpl(
    BlobStorageContext* context,
    scoped_refptr<FileSystemContext> file_system_context)
    : context_(context),
      file_system_context_(std::move(file_system_context)),
      weak_ptr_factory_(this) {}

BlobRegistryImpl::~BlobRegistryImpl() {}

void BlobRegistryImpl::Bind(mojom::BlobRegistryRequest request,
                            std::unique_ptr<Delegate> delegate) {
  DCHECK(delegate);
  bindings_.AddBinding(this, std::move(request), std::move(delegate));
}

void BlobRegistryImpl::Register(mojom::BlobRequest blob,
                                const std::string& uuid,
                                const std::string& content_type,
                                const std::string& content_disposition,
                                std::vector<mojom::DataElementPtr> elements,
                                RegisterCallback callback) {
  if (uuid.empty() || context_->registry().HasEntry(uuid) ||
      base::ContainsKey(blobs_under_construction_, uuid)) {
    bindings_.ReportBadMessage("Invalid UUID passed to BlobRegistry::Register");
    return;
  }

  Delegate* delegate = bindings_.dispatch_context().get();
  DCHECK(delegate);
  for (const auto& element : elements) {
    if (element->is_file()) {
      if (!delegate->CanReadFile(element->get_file()->path)) {
        std::unique_ptr<BlobDataHandle> handle =
            context_->AddBrokenBlob(uuid, content_type, content_disposition,
                                    BlobStatus::ERR_FILE_WRITE_FAILED);
        BlobImpl::Create(std::move(handle), std::move(blob));
        std::move(callback).Run();
        return;
      }
    } else if (element->is_file_filesystem()) {
      FileSystemURL filesystem_url(
          file_system_context_->CrackURL(element->get_file_filesystem()->url));
      if (!filesystem_url.is_valid() ||
          !file_system_context_->GetFileSystemBackend(filesystem_url.type()) ||
          !delegate->CanReadFileSystemFile(filesystem_url)) {
        std::unique_ptr<BlobDataHandle> handle =
            context_->AddBrokenBlob(uuid, content_type, content_disposition,
                                    BlobStatus::ERR_FILE_WRITE_FAILED);
        BlobImpl::Create(std::move(handle), std::move(blob));
        std::move(callback).Run();
        return;
      }
    }
  }

  blobs_under_construction_[uuid] = base::MakeUnique<BlobUnderConstruction>(
      this, uuid, content_type, content_disposition, std::move(elements),
      bindings_.GetBadMessageCallback());

  std::unique_ptr<BlobDataHandle> handle =
      context_->AddFutureBlob(uuid, content_type, content_disposition);
  BlobImpl::Create(std::move(handle), std::move(blob));

  blobs_under_construction_[uuid]->StartTransportation();

  std::move(callback).Run();
}

void BlobRegistryImpl::GetBlobFromUUID(mojom::BlobRequest blob,
                                       const std::string& uuid) {
  if (uuid.empty()) {
    bindings_.ReportBadMessage(
        "Invalid UUID passed to BlobRegistry::GetBlobFromUUID");
    return;
  }
  if (!context_->registry().HasEntry(uuid)) {
    // TODO(mek): Log histogram, old code logs Storage.Blob.InvalidReference
    return;
  }
  BlobImpl::Create(context_->GetBlobDataFromUUID(uuid), std::move(blob));
}

}  // namespace storage
