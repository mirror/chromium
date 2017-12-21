// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/resource_request_body.h"

#if defined(OS_ANDROID)
#include "content/common/android/resource_request_body_android.h"
#endif

namespace content {

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

#if defined(OS_ANDROID)
base::android::ScopedJavaLocalRef<jobject> ResourceRequestBody::ToJavaObject(
    JNIEnv* env) {
  return ConvertResourceRequestBodyToJavaObject(
      env, static_cast<ResourceRequestBody*>(this));
}

// static
scoped_refptr<ResourceRequestBody> ResourceRequestBody::FromJavaObject(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_object) {
  return ExtractResourceRequestBodyFromJavaObject(env, java_object);
}
#endif

void ResourceRequestBody::AppendBytes(const char* bytes, int bytes_len) {
  if (bytes_len > 0) {
    elements_.push_back(Element());
    elements_.back().SetToBytes(bytes, bytes_len);
  }
}

void ResourceRequestBody::AppendFileRange(
    const base::FilePath& file_path,
    uint64_t offset,
    uint64_t length,
    const base::Time& expected_modification_time) {
  elements_.push_back(Element());
  elements_.back().SetToFilePathRange(file_path, offset, length,
                                      expected_modification_time);
}

void ResourceRequestBody::AppendRawFileRange(
    base::File file,
    const base::FilePath& file_path,
    uint64_t offset,
    uint64_t length,
    const base::Time& expected_modification_time) {
  elements_.push_back(Element());
  elements_.back().SetToFileRange(std::move(file), file_path, offset, length,
                                  expected_modification_time);
}

void ResourceRequestBody::AppendBlob(const std::string& uuid) {
  elements_.push_back(Element());
  elements_.back().SetToBlob(uuid);
}

void ResourceRequestBody::AppendFileSystemFileRange(
    const GURL& url,
    uint64_t offset,
    uint64_t length,
    const base::Time& expected_modification_time) {
  elements_.push_back(Element());
  elements_.back().SetToFileSystemUrlRange(url, offset, length,
                                           expected_modification_time);
}

void ResourceRequestBody::AppendDataPipe(
    network::mojom::DataPipeGetterPtr data_pipe_getter) {
  elements_.push_back(Element());
  elements_.back().SetToDataPipe(std::move(data_pipe_getter));
}

std::vector<base::FilePath> ResourceRequestBody::GetReferencedFiles() const {
  std::vector<base::FilePath> result;
  for (const auto& element : *elements()) {
    if (element.type() == Element::TYPE_FILE)
      result.push_back(element.path());
  }
  return result;
}

scoped_refptr<ResourceRequestBody> ResourceRequestBody::Clone() const {
  auto clone = base::MakeRefCounted<ResourceRequestBody>();

  clone->identifier_ = identifier_;
  clone->contains_sensitive_info_ = contains_sensitive_info_;
  for (const Element& element : elements_) {
    Element cloned_element;
    switch (element.type()) {
      case Element::TYPE_UNKNOWN:
      case Element::TYPE_DISK_CACHE_ENTRY:
      case Element::TYPE_BYTES_DESCRIPTION:
        NOTREACHED();
        continue;
      case Element::TYPE_DATA_PIPE: {
        network::mojom::DataPipeGetterPtrInfo clone_ptr_info;
        element.data_pipe()->Clone(mojo::MakeRequest(&clone_ptr_info));
        network::mojom::DataPipeGetterPtr clone_ptr(std::move(clone_ptr_info));
        cloned_element.SetToDataPipe(std::move(clone_ptr));
        break;
      }
      case Element::TYPE_RAW_FILE:
        cloned_element.SetToFileRange(
            element.file().Duplicate(), element.path(), element.offset(),
            element.length(), element.expected_modification_time());
        break;
      case Element::TYPE_FILE_FILESYSTEM:
        cloned_element.SetToFileSystemUrlRange(
            element.filesystem_url(), element.offset(), element.length(),
            element.expected_modification_time());
        break;
      case Element::TYPE_BLOB:
        cloned_element.SetToBlobRange(element.blob_uuid(), element.offset(),
                                      element.length());
        break;
      case Element::TYPE_FILE:
        cloned_element.SetToFilePathRange(element.path(), element.offset(),
                                          element.length(),
                                          element.expected_modification_time());
        break;
      case Element::TYPE_BYTES:
        cloned_element.SetToBytes(element.bytes(), element.length());
        break;
    }
    clone->elements_.push_back(std::move(cloned_element));
  }

  return clone;
}

ResourceRequestBody::~ResourceRequestBody() {}

}  // namespace content
