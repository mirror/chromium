// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/blob/blob_registry_impl.h"

#include "base/barrier_closure.h"
#include "base/callback_helpers.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_impl.h"
#include "storage/browser/blob/blob_storage_context.h"

namespace storage {

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
        referenced_blob_uuids_(elements_.size()),
        weak_ptr_factory_(this) {
    builder_.set_content_type(content_type);
    builder_.set_content_disposition(content_disposition);
  }

  // Call this after constructing to kick of fetching of UUIDs of blobs
  // referenced by this new blob. This (and any further methods) could end up
  // deleting |this| by removing it from the blobs_under_construction_
  // collection in the blob service.
  void StartFetchingBlobUUIDs();

  ~BlobUnderConstruction() {
    // Mark any blobs that were waiting for this one as broken.
    for (const auto& p : blobs_waiting_for_uuids_) {
      if (!p)
        continue;
      p->MarkAsBroken(BlobStatus::ERR_REFERENCED_BLOB_BROKEN);
    }
  }

  const std::string& uuid() const { return builder_.uuid(); }

 private:
  BlobStorageContext* context() const { return blob_registry_->context_; }

  // Marks this blob as broken. If an optional |bad_message_reason| is provided,
  // this will also report a BadMessage on the binding over which the initial
  // Register request was received.
  // Also deletes |this| by removing it from the blobs_under_construction_ list.
  void MarkAsBroken(BlobStatus reason,
                    const std::string& bad_message_reason = "") {
    context()->CancelBuildingBlob(uuid(), reason);
    if (!bad_message_reason.empty())
      std::move(bad_message_callback_).Run(bad_message_reason);
    blob_registry_->blobs_under_construction_.erase(uuid());
  }

  // Called when the UUID of a referenced blob is received.
  void ReceivedBlobUUID(size_t element_index, const std::string& uuid);

  // Called by either StartFetchingBlobUUIDs or ReceivedBlobUUID when all the
  // UUIDs of referenced blobs have been resolved. Starts checking for circular
  // references. Before we can proceed with actually building the blob, all
  // referenced blobs also need to have resolved their referenced blobs (to
  // always be able to calculate the size of the newly built blob). To ensure
  // this we might have to wait for one or more possibly indirectly dependent
  // blobs to also have resolved the UUIDs of their dependencies. This waiting
  // is kicked of by this method.
  void ResolvedAllBlobUUIDs();

  // Helper method to check for circular references. This returns true if
  // |blob| is the same blob as this, or if any of its dependencies are the same
  // blob as this. If this methods returns false that does not mean that no
  // circular references exist. It may simply be the case that some dependencies
  // have not resolved all their dependencies yet, in which case calls to
  // ResolvedDependentBlobUUIDs will be queued up which will do further
  // verification.
  bool ContainsReferencesToThis(BlobUnderConstruction* blob);

  // Called when one of our dependent blobs has resolved its dependent UUIDs.
  void DependentBlobResolvedBlobUUIDs(BlobUnderConstruction* blob);

  // Called when all blob dependencies have been resolved, and we're sure there
  // are no circular dependencies. This finally kicks of the actually building
  // of the blob, and figures out how to transport any bytes that might need
  // transporting.
  void ResolvedAllBlobDependencies();

 private:
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

  // List of UUIDs for referenced blobs. Same size as |elements_|. All entries
  // for non-blob elements will remain empty strings.
  std::vector<std::string> referenced_blob_uuids_;

  // Number of blob UUIDs that still need to be resolved.
  size_t unresolved_blob_uuid_count_ = 0;

  // Number of dependent blobs who haven't resolved their dependent blobs UUIDs
  // yet.
  size_t dependent_blobs_with_unresolved_uuids_count_ = 0;

  // List of UUIDs of blobs for which either it has been determined that they
  // don't contain references to this blob, or we're still waiting on those
  // blobs to resolve their referenced blob UUIDs.
  std::set<BlobUnderConstruction*> verified_circularity_blobs_;

  // List of blobs that are waiting for this blob to resolve its UUIDs.
  std::vector<base::WeakPtr<BlobUnderConstruction>> blobs_waiting_for_uuids_;

  base::WeakPtrFactory<BlobUnderConstruction> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(BlobUnderConstruction);
};

void BlobRegistryImpl::BlobUnderConstruction::StartFetchingBlobUUIDs() {
  for (size_t i = 0; i < elements_.size(); ++i) {
    const auto& element = elements_[i];
    if (element->is_blob()) {
      if (element->get_blob()->blob.encountered_error()) {
        // Will delete |this|.
        MarkAsBroken(BlobStatus::ERR_REFERENCED_BLOB_BROKEN);
        return;
      }

      unresolved_blob_uuid_count_++;

      // If connection to blob is broken, something bad happened, so mark this
      // new blob as broken, which will delete |this| and keep it from doing
      // unneeded extra work.
      element->get_blob()->blob.set_connection_error_handler(base::BindOnce(
          &BlobUnderConstruction::MarkAsBroken, weak_ptr_factory_.GetWeakPtr(),
          BlobStatus::ERR_REFERENCED_BLOB_BROKEN, ""));

      element->get_blob()->blob->GetInternalUUID(
          base::BindOnce(&BlobUnderConstruction::ReceivedBlobUUID,
                         weak_ptr_factory_.GetWeakPtr(), i));
    }
  }

  // TODO(mek): Do we need some kind of timeout for fetching the UUIDs?
  // Without it a blob could forever remaing pending if a renderer sends us
  // a BlobPtr connected to a (malicious) non-responding implementation.

  // If there were no unresolved blobs, immediately proceed to the next step.
  // Currently this will only happen if there are no blobs referenced
  // whatsoever, but hopefully in the future blob UUIDs will be cached in the
  // message pipe handle, making things much more efficient in the common case.
  if (unresolved_blob_uuid_count_ == 0)
    ResolvedAllBlobUUIDs();
}

void BlobRegistryImpl::BlobUnderConstruction::ReceivedBlobUUID(
    size_t element_index,
    const std::string& uuid) {
  DCHECK(elements_[element_index]->is_blob());
  DCHECK(referenced_blob_uuids_[element_index].empty());
  DCHECK_GT(unresolved_blob_uuid_count_, 0u);

  referenced_blob_uuids_[element_index] = uuid;
  if (--unresolved_blob_uuid_count_ == 0)
    ResolvedAllBlobUUIDs();
}

void BlobRegistryImpl::BlobUnderConstruction::ResolvedAllBlobUUIDs() {
  DCHECK_EQ(unresolved_blob_uuid_count_, 0u);

  // Loop over all referenced blobs, and make sure we actually got a valid blob
  // reference. Also verify that non of our referenced blobs contains references
  // to this blob.
  for (size_t i = 0; i < elements_.size(); ++i) {
    if (elements_[i]->is_blob()) {
      const std::string& blob_uuid = referenced_blob_uuids_[i];
      if (blob_uuid.empty() || !context()->registry().HasEntry(blob_uuid)) {
        // Will delete |this|.
        MarkAsBroken(BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS,
                     "Bad blob references in BlobRegistry::Register");
        return;
      }

      auto it = blob_registry_->blobs_under_construction_.find(blob_uuid);
      // Referenced blobs that aren't currently being constructed can be safely
      // ignored, as any potential reference cycles involving those should be
      // impossible, and if they exist anyway will be caught when
      // BlobStorageContext tries to determine the size of this new blob.
      if (it == blob_registry_->blobs_under_construction_.end())
        continue;

      if (ContainsReferencesToThis(it->second.get())) {
        // Will delete |this|.
        MarkAsBroken(BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS,
                     "Circular blob references in BlobRegistry::Register");
        return;
      }
    }
  }

  // Inform any blobs that were waiting for this blob to resolve its UUIDs that
  // all of the UUIDs have now in fact been resolved.
  for (const auto& p : blobs_waiting_for_uuids_) {
    if (!p)
      continue;
    p->DependentBlobResolvedBlobUUIDs(this);
  }

  // If checking for circular references has completed, continue with building
  // the blob.
  if (dependent_blobs_with_unresolved_uuids_count_ == 0)
    ResolvedAllBlobDependencies();
}

bool BlobRegistryImpl::BlobUnderConstruction::ContainsReferencesToThis(
    BlobUnderConstruction* blob) {
  if (blob == this)
    return true;

  // If an entry already exists, either no self references were found, or we're
  // already waiting for results from the referenced blob. Either way we're done
  // for now.
  if (!verified_circularity_blobs_.insert(blob).second)
    return false;

  // If the referenced blob has any unresolved blob UUIDs, we wait for it to
  // finish resolving its UUIDs. Returns false, since no self references have
  // been detected, the blob will be checked again when its dependencies have
  // been resolved,
  if (blob->unresolved_blob_uuid_count_ > 0) {
    dependent_blobs_with_unresolved_uuids_count_++;
    blob->blobs_waiting_for_uuids_.push_back(weak_ptr_factory_.GetWeakPtr());
    return false;
  }

  // Finally check if any of the blobs dependencies, or recursively their
  // depencies are |this|.
  for (const std::string& referenced_blob_uuid : blob->referenced_blob_uuids_) {
    // Empty strings just mean that the corresponding element is not a blob.
    if (referenced_blob_uuid.empty())
      continue;

    // Referenced blobs that aren't currently being constructed can be safely
    // ignored, as any potential reference cycles involving those should be
    // impossible, and if they exist anyway will be caught when
    // BlobStorageContext tries to determine the size of this new blob.
    auto it =
        blob_registry_->blobs_under_construction_.find(referenced_blob_uuid);
    if (it == blob_registry_->blobs_under_construction_.end())
      continue;

    if (ContainsReferencesToThis(it->second.get()))
      return true;
  }

  // No self references found.
  return false;
}

void BlobRegistryImpl::BlobUnderConstruction::DependentBlobResolvedBlobUUIDs(
    BlobUnderConstruction* blob) {
  DCHECK_EQ(blob->unresolved_blob_uuid_count_, 0u);

  dependent_blobs_with_unresolved_uuids_count_--;

  // Remove from already checked blobs, and check again.
  verified_circularity_blobs_.erase(blob);
  if (ContainsReferencesToThis(blob)) {
    // Will delete |this|.
    MarkAsBroken(BlobStatus::ERR_INVALID_CONSTRUCTION_ARGUMENTS,
                 "Circular blob references in BlobRegistry::Register");
    return;
  }

  // If no more dependencies with unresolved dependencies exist, continue with
  // building the blob.
  if (dependent_blobs_with_unresolved_uuids_count_ == 0)
    ResolvedAllBlobDependencies();
}

void BlobRegistryImpl::BlobUnderConstruction::ResolvedAllBlobDependencies() {
  DCHECK_EQ(unresolved_blob_uuid_count_, 0u);
  DCHECK_EQ(dependent_blobs_with_unresolved_uuids_count_, 0u);

  // TODO(mek): Fill BlobDataBuilder with elements_.
  std::unique_ptr<BlobDataHandle> new_handle =
      context()->BuildPreregisteredBlob(
          builder_, BlobStorageContext::TransportAllowedCallback());

  // TODO(mek): Update BlobImpl with new BlobDataHandle. Although handles
  // only differ in their size() attribute, which is currently not used by
  // BlobImpl.
  DCHECK_EQ(BlobStatus::DONE, new_handle->GetBlobStatus());
}

BlobRegistryImpl::BlobRegistryImpl(BlobStorageContext* context)
    : context_(context), weak_ptr_factory_(this) {}

BlobRegistryImpl::~BlobRegistryImpl() {}

void BlobRegistryImpl::Bind(mojom::BlobRegistryRequest request) {
  bindings_.AddBinding(this, std::move(request));
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

  // TODO(mek): Security policy checks for files and filesystem items.

  blobs_under_construction_[uuid] = base::MakeUnique<BlobUnderConstruction>(
      this, uuid, content_type, content_disposition, std::move(elements),
      bindings_.GetBadMessageCallback());

  std::unique_ptr<BlobDataHandle> handle =
      context_->AddFutureBlob(uuid, content_type, content_disposition);
  BlobImpl::Create(std::move(handle), std::move(blob));

  blobs_under_construction_[uuid]->StartFetchingBlobUUIDs();

  std::move(callback).Run();
}

void BlobRegistryImpl::GetBlobFromUUID(mojom::BlobRequest blob,
                                       const std::string& uuid) {
  if (uuid.empty()) {
    bindings_.ReportBadMessage("Invalid UUID passed to BlobRegistry::GetBlobFromUUID");
    return;
  }
  if (!context_->registry().HasEntry(uuid)) {
    // TODO(mek): Log histogram, old code logs Storage.Blob.InvalidReference
    return;
  }
  BlobImpl::Create(context_->GetBlobDataFromUUID(uuid), std::move(blob));
}

}  // namespace storage
