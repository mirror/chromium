// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/resource_request_body.h"

namespace network {

ResourceRequestBody::ResourceRequestBody()
    : identifier_(0), contains_sensitive_info_(false) {}

// static
scoped_refptr<ResourceRequestBody> ResourceRequestBody::CreateFromBytes(
    const char* bytes,
    size_t length) {
  scoped_refptr<ResourceRequestBody> result = new ResourceRequestBody();
  result->AppendBytes(bytes, length);
  return result;
}

void ResourceRequestBody::AppendBytes(const char* bytes, int bytes_len) {
  if (bytes_len > 0) {
    elements_.push_back(DataElement());
    elements_.back().SetToBytes(bytes, bytes_len);
  }
}

void ResourceRequestBody::AppendFileRange(
    const base::FilePath& file_path,
    uint64_t offset,
    uint64_t length,
    const base::Time& expected_modification_time) {
  elements_.push_back(DataElement());
  elements_.back().SetToFilePathRange(file_path, offset, length,
                                      expected_modification_time);
}

void ResourceRequestBody::AppendRawFileRange(
    base::File file,
    const base::FilePath& file_path,
    uint64_t offset,
    uint64_t length,
    const base::Time& expected_modification_time) {
  elements_.push_back(DataElement());
  elements_.back().SetToFileRange(std::move(file), file_path, offset, length,
                                  expected_modification_time);
}

void ResourceRequestBody::AppendBlob(const std::string& uuid) {
  elements_.push_back(DataElement());
  elements_.back().SetToBlob(uuid);
}

void ResourceRequestBody::AppendFileSystemFileRange(
    const GURL& url,
    uint64_t offset,
    uint64_t length,
    const base::Time& expected_modification_time) {
  elements_.push_back(DataElement());
  elements_.back().SetToFileSystemUrlRange(url, offset, length,
                                           expected_modification_time);
}

void ResourceRequestBody::AppendDataPipe(
    mojom::DataPipeGetterPtr data_pipe_getter) {
  elements_.push_back(DataElement());
  elements_.back().SetToDataPipe(std::move(data_pipe_getter));
}

std::vector<base::FilePath> ResourceRequestBody::GetReferencedFiles() const {
  std::vector<base::FilePath> result;
  for (const auto& element : *elements()) {
    if (element.type() == DataElement::TYPE_FILE)
      result.push_back(element.path());
  }
  return result;
}

scoped_refptr<ResourceRequestBody> ResourceRequestBody::Clone() const {
  auto clone = base::MakeRefCounted<ResourceRequestBody>();

  clone->identifier_ = identifier_;
  clone->contains_sensitive_info_ = contains_sensitive_info_;
  for (const DataElement& element : elements_) {
    DataElement cloned_element;
    switch (element.type()) {
      case DataElement::TYPE_UNKNOWN:
      case DataElement::TYPE_DISK_CACHE_ENTRY:
      case DataElement::TYPE_BYTES_DESCRIPTION:
        NOTREACHED();
        break;
      case DataElement::TYPE_DATA_PIPE: {
        network::mojom::DataPipeGetterPtrInfo clone_ptr_info;
        element.data_pipe()->Clone(mojo::MakeRequest(&clone_ptr_info));
        network::mojom::DataPipeGetterPtr clone_ptr(std::move(clone_ptr_info));
        cloned_element.SetToDataPipe(std::move(clone_ptr));
        break;
      }
      case DataElement::TYPE_RAW_FILE:
        cloned_element.SetToFileRange(
            element.file().Duplicate(), element.path(), element.offset(),
            element.length(), element.expected_modification_time());
        break;
      case DataElement::TYPE_FILE_FILESYSTEM:
        cloned_element.SetToFileSystemUrlRange(
            element.filesystem_url(), element.offset(), element.length(),
            element.expected_modification_time());
        break;
      case DataElement::TYPE_BLOB:
        NOTREACHED() << "Clone() is only used for NetworkService. There should "
                        "be no blob elements in NetworkService";
        break;
      case DataElement::TYPE_FILE:
        cloned_element.SetToFilePathRange(element.path(), element.offset(),
                                          element.length(),
                                          element.expected_modification_time());
        break;
      case DataElement::TYPE_BYTES:
        cloned_element.SetToBytes(element.bytes(), element.length());
        break;
    }
    clone->elements_.push_back(std::move(cloned_element));
  }

  return clone;
}

ResourceRequestBody::~ResourceRequestBody() {}

}  // namespace network
