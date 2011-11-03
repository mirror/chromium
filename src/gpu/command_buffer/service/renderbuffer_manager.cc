// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/renderbuffer_manager.h"
#include "base/logging.h"
#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"

namespace gpu {
namespace gles2 {

RenderbufferManager::RenderbufferManager(
    GLint max_renderbuffer_size, GLint max_samples)
    : max_renderbuffer_size_(max_renderbuffer_size),
      max_samples_(max_samples),
      num_uncleared_renderbuffers_(0)  {
}

RenderbufferManager::~RenderbufferManager() {
  DCHECK(renderbuffer_infos_.empty());
}

void RenderbufferManager::Destroy(bool have_context) {
  while (!renderbuffer_infos_.empty()) {
    if (have_context) {
      RenderbufferInfo* info = renderbuffer_infos_.begin()->second;
      if (!info->IsDeleted()) {
        GLuint service_id = info->service_id();
        glDeleteRenderbuffersEXT(1, &service_id);
        info->MarkAsDeleted();
      }
    }
    renderbuffer_infos_.erase(renderbuffer_infos_.begin());
  }
}

void RenderbufferManager::SetInfo(
  RenderbufferInfo* renderbuffer,
  GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
  DCHECK(renderbuffer);
  if (!renderbuffer->cleared()) {
    --num_uncleared_renderbuffers_;
  }
  renderbuffer->SetInfo(samples, internalformat, width, height);
  if (!renderbuffer->cleared()) {
    ++num_uncleared_renderbuffers_;
  }
}

void RenderbufferManager::SetCleared(RenderbufferInfo* renderbuffer) {
  DCHECK(renderbuffer);
  if (!renderbuffer->cleared()) {
    --num_uncleared_renderbuffers_;
  }
  renderbuffer->set_cleared();
  if (!renderbuffer->cleared()) {
    ++num_uncleared_renderbuffers_;
  }
}

void RenderbufferManager::CreateRenderbufferInfo(
    GLuint client_id, GLuint service_id) {
  RenderbufferInfo::Ref info(new RenderbufferInfo(service_id));
  std::pair<RenderbufferInfoMap::iterator, bool> result =
      renderbuffer_infos_.insert(std::make_pair(client_id, info));
  DCHECK(result.second);
  if (!info->cleared()) {
    ++num_uncleared_renderbuffers_;
  }
}

RenderbufferManager::RenderbufferInfo* RenderbufferManager::GetRenderbufferInfo(
    GLuint client_id) {
  RenderbufferInfoMap::iterator it = renderbuffer_infos_.find(client_id);
  return it != renderbuffer_infos_.end() ? it->second : NULL;
}

void RenderbufferManager::RemoveRenderbufferInfo(GLuint client_id) {
  RenderbufferInfoMap::iterator it = renderbuffer_infos_.find(client_id);
  if (it != renderbuffer_infos_.end()) {
    RenderbufferInfo* info = it->second;
    if (!info->cleared()) {
      --num_uncleared_renderbuffers_;
    }
    info->MarkAsDeleted();
    renderbuffer_infos_.erase(it);
  }
}

bool RenderbufferManager::GetClientId(
    GLuint service_id, GLuint* client_id) const {
  // This doesn't need to be fast. It's only used during slow queries.
  for (RenderbufferInfoMap::const_iterator it = renderbuffer_infos_.begin();
       it != renderbuffer_infos_.end(); ++it) {
    if (it->second->service_id() == service_id) {
      *client_id = it->first;
      return true;
    }
  }
  return false;
}

}  // namespace gles2
}  // namespace gpu


