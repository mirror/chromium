// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/share_group.h"

#include <stdint.h>

#include <vector>

#include "base/containers/stack.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "gpu/command_buffer/client/cmd_buffer_helper.h"
#include "gpu/command_buffer/client/program_info_manager.h"
#include "gpu/command_buffer/common/id_allocator.h"

namespace gpu {
namespace gles2 {

ShareGroupContextData::IdHandlerData::IdHandlerData() : flush_generation_(0) {}
ShareGroupContextData::IdHandlerData::~IdHandlerData() = default;

static_assert(gpu::kInvalidResource == 0,
              "GL expects kInvalidResource to be 0");

// The standard id handler.
class IdHandler : public IdHandlerInterface {
 public:
  IdHandler() = default;
  ~IdHandler() override = default;

  // Overridden from IdHandlerInterface.
  void MakeIds(CommandBufferHelper* /* helper */,
               ShareGroupContextData* /* ctxt_data */,
               GLuint id_offset,
               GLsizei n,
               GLuint* ids) override {
    base::AutoLock auto_lock(lock_);
    if (id_offset == 0) {
      for (GLsizei ii = 0; ii < n; ++ii) {
        ids[ii] = id_allocator_.AllocateID();
      }
    } else {
      for (GLsizei ii = 0; ii < n; ++ii) {
        ids[ii] = id_allocator_.AllocateIDAtOrAbove(id_offset);
        id_offset = ids[ii] + 1;
      }
    }
  }

  // Overridden from IdHandlerInterface.
  bool FreeIds(CommandBufferHelper* helper,
               ShareGroupContextData* /* ctxt_data */,
               GLsizei n,
               const GLuint* ids,
               DeleteFn delete_fn) override {
    base::AutoLock auto_lock(lock_);

    for (GLsizei ii = 0; ii < n; ++ii) {
      id_allocator_.FreeID(ids[ii]);
    }

    std::move(delete_fn).Run(n, ids);
    // We need to ensure that the delete call is evaluated on the service side
    // before any other contexts issue commands using these client ids.
    helper->OrderingBarrier();
    return true;
  }

  // Overridden from IdHandlerInterface.
  bool MarkAsUsedForBind(GLenum target, GLuint id, BindFn bind_fn) override {
    base::AutoLock auto_lock(lock_);
    bool result = id ? id_allocator_.MarkAsUsed(id) : true;
    std::move(bind_fn).Run(target, id);
    return result;
  }
  bool MarkAsUsedForBind(GLenum target,
                         GLuint index,
                         GLuint id,
                         BindIndexedFn bind_fn) override {
    base::AutoLock auto_lock(lock_);
    bool result = id ? id_allocator_.MarkAsUsed(id) : true;
    std::move(bind_fn).Run(target, index, id);
    return result;
  }
  bool MarkAsUsedForBind(GLenum target,
                         GLuint index,
                         GLuint id,
                         GLintptr offset,
                         GLsizeiptr size,
                         BindIndexedRangeFn bind_fn) override {
    base::AutoLock auto_lock(lock_);
    bool result = id ? id_allocator_.MarkAsUsed(id) : true;
    std::move(bind_fn).Run(target, index, id, offset, size);
    return result;
  }

  void FreeContext(CommandBufferHelper* helper,
                   ShareGroupContextData* ctxt_data) override {}

 private:
  base::Lock lock_;
  IdAllocator id_allocator_;
};

// An id handler that requires Gen before Bind.
class StrictIdHandler : public IdHandlerInterface {
 public:
  explicit StrictIdHandler(int id_namespace) : id_namespace_(id_namespace) {}
  ~StrictIdHandler() override = default;

  // Overridden from IdHandler.
  void MakeIds(CommandBufferHelper* helper,
               ShareGroupContextData* ctxt_data,
               GLuint /* id_offset */,
               GLsizei n,
               GLuint* ids) override {
    base::AutoLock auto_lock(lock_);

    // Collect pending FreeIds from other flush_generation.
    CollectPendingFreeIds(helper, ctxt_data);

    for (GLsizei ii = 0; ii < n; ++ii) {
      if (!free_ids_.empty()) {
        // Allocate a previously freed Id.
        ids[ii] = free_ids_.top();
        free_ids_.pop();

        // Record kIdInUse state.
        DCHECK(id_states_[ids[ii] - 1] == kIdFree);
        id_states_[ids[ii] - 1] = kIdInUse;
      } else {
        // Allocate a new Id.
        id_states_.push_back(kIdInUse);
        ids[ii] = id_states_.size();
      }
    }
  }

  // Overridden from IdHandler.
  bool FreeIds(CommandBufferHelper* helper,
               ShareGroupContextData* ctxt_data,
               GLsizei n,
               const GLuint* ids,
               DeleteFn delete_fn) override {
    // Delete stub must run before CollectPendingFreeIds.
    std::move(delete_fn).Run(n, ids);

    {
      base::AutoLock auto_lock(lock_);

      // Collect pending FreeIds from other flush_generation.
      CollectPendingFreeIds(helper, ctxt_data);

      // Save Ids to free in a later flush_generation.
      ShareGroupContextData::IdHandlerData* handler_data =
          ctxt_data->id_handler_data(id_namespace_);

      for (GLsizei ii = 0; ii < n; ++ii) {
        GLuint id = ids[ii];
        if (id != 0) {
          // Save freed Id for later.
          DCHECK(id_states_[id - 1] == kIdInUse);
          id_states_[id - 1] = kIdPendingFree;
          handler_data->freed_ids_.push_back(id);
        }
      }
    }

    return true;
  }

  // Overridden from IdHandler.
  bool MarkAsUsedForBind(GLenum target, GLuint id, BindFn bind_fn) override {
#ifndef NDEBUG
    if (id != 0) {
      base::AutoLock auto_lock(lock_);
      DCHECK(id_states_[id - 1] == kIdInUse);
    }
#endif
    // StrictIdHandler is used if |bind_generates_resource| is false. In that
    // case, |bind_fn| will not use Flush() after helper->Bind*(), so it is OK
    // to call |bind_fn| without holding the lock.
    std::move(bind_fn).Run(target, id);
    return true;
  }
  bool MarkAsUsedForBind(GLenum target,
                         GLuint index,
                         GLuint id,
                         BindIndexedFn bind_fn) override {
#ifndef NDEBUG
    if (id != 0) {
      base::AutoLock auto_lock(lock_);
      DCHECK(id_states_[id - 1] == kIdInUse);
    }
#endif
    // StrictIdHandler is used if |bind_generates_resource| is false. In that
    // case, |bind_fn| will not use Flush() after helper->Bind*(), so it is OK
    // to call |bind_fn| without holding the lock.
    std::move(bind_fn).Run(target, index, id);
    return true;
  }
  bool MarkAsUsedForBind(GLenum target,
                         GLuint index,
                         GLuint id,
                         GLintptr offset,
                         GLsizeiptr size,
                         BindIndexedRangeFn bind_fn) override {
#ifndef NDEBUG
    if (id != 0) {
      base::AutoLock auto_lock(lock_);
      DCHECK(id_states_[id - 1] == kIdInUse);
    }
#endif
    // StrictIdHandler is used if |bind_generates_resource| is false. In that
    // case, |bind_fn| will not use Flush() after helper->Bind*(), so it is OK
    // to call |bind_fn| without holding the lock.
    std::move(bind_fn).Run(target, index, id, offset, size);
    return true;
  }

  // Overridden from IdHandlerInterface.
  void FreeContext(CommandBufferHelper* helper,
                   ShareGroupContextData* ctxt_data) override {
    base::AutoLock auto_lock(lock_);
    CollectPendingFreeIds(helper, ctxt_data);
  }

 private:
  enum IdState { kIdFree, kIdPendingFree, kIdInUse };

  void CollectPendingFreeIds(CommandBufferHelper* helper,
                             ShareGroupContextData* ctxt_data) {
    uint32_t flush_generation = helper->flush_generation();
    ShareGroupContextData::IdHandlerData* handler_data =
        ctxt_data->id_handler_data(id_namespace_);

    if (handler_data->flush_generation_ != flush_generation) {
      handler_data->flush_generation_ = flush_generation;
      for (uint32_t ii = 0; ii < handler_data->freed_ids_.size(); ++ii) {
        const GLuint id = handler_data->freed_ids_[ii];
        DCHECK(id_states_[id - 1] == kIdPendingFree);
        id_states_[id - 1] = kIdFree;
        free_ids_.push(id);
      }
      handler_data->freed_ids_.clear();
    }
  }

  int id_namespace_;

  base::Lock lock_;
  std::vector<uint8_t> id_states_;
  base::stack<uint32_t> free_ids_;
};

// An id handler for ids that are never reused.
class NonReusedIdHandler : public IdHandlerInterface {
 public:
  NonReusedIdHandler() : last_id_(0) {}
  ~NonReusedIdHandler() override = default;

  // Overridden from IdHandlerInterface.
  void MakeIds(CommandBufferHelper* /* helper */,
               ShareGroupContextData* /* ctxt_data */,
               GLuint id_offset,
               GLsizei n,
               GLuint* ids) override {
    base::AutoLock auto_lock(lock_);
    for (GLsizei ii = 0; ii < n; ++ii) {
      ids[ii] = ++last_id_ + id_offset;
    }
  }

  // Overridden from IdHandlerInterface.
  bool FreeIds(CommandBufferHelper* /* helper */,
               ShareGroupContextData* /* ctxt_data */,
               GLsizei n,
               const GLuint* ids,
               DeleteFn delete_fn) override {
    // Ids are never freed.
    std::move(delete_fn).Run(n, ids);
    return true;
  }

  // Overridden from IdHandlerInterface.
  bool MarkAsUsedForBind(GLenum /* target */,
                         GLuint /* id */,
                         BindFn /* bind_fn */) override {
    // This is only used for Shaders and Programs which have no bind.
    return false;
  }
  bool MarkAsUsedForBind(GLenum /* target */,
                         GLuint /* index */,
                         GLuint /* id */,
                         BindIndexedFn /* bind_fn */) override {
    // This is only used for Shaders and Programs which have no bind.
    return false;
  }
  bool MarkAsUsedForBind(GLenum /* target */,
                         GLuint /* index */,
                         GLuint /* id */,
                         GLintptr /* offset */,
                         GLsizeiptr /* size */,
                         BindIndexedRangeFn /* bind_fn */) override {
    // This is only used for Shaders and Programs which have no bind.
    return false;
  }

  void FreeContext(CommandBufferHelper* helper,
                   ShareGroupContextData* ctxt_data) override {}

 private:
  base::Lock lock_;
  GLuint last_id_;
};

class RangeIdHandler : public RangeIdHandlerInterface {
 public:
  RangeIdHandler() = default;

  void MakeIdRange(CommandBufferHelper* /* helper */,
                   GLsizei n,
                   GLuint* first_id) override {
    base::AutoLock auto_lock(lock_);
    *first_id = id_allocator_.AllocateIDRange(n);
  }

  void FreeIdRange(CommandBufferHelper* helper,
                   const GLuint first_id,
                   GLsizei range,
                   DeleteRangeFn delete_fn) override {
    base::AutoLock auto_lock(lock_);
    DCHECK(range > 0);
    id_allocator_.FreeIDRange(first_id, range);
    std::move(delete_fn).Run(first_id, range);
    helper->OrderingBarrier();
  }

  void FreeContext(CommandBufferHelper* helper) override {}

 private:
  base::Lock lock_;
  IdAllocator id_allocator_;
};

ShareGroup::ShareGroup(bool bind_generates_resource, uint64_t tracing_guid)
    : bind_generates_resource_(bind_generates_resource),
      tracing_guid_(tracing_guid) {
  if (bind_generates_resource) {
    for (int i = 0;
         i < static_cast<int>(SharedIdNamespaces::kNumSharedIdNamespaces);
         ++i) {
      if (i == static_cast<int>(SharedIdNamespaces::kProgramsAndShaders)) {
        id_handlers_[i].reset(new NonReusedIdHandler());
      } else {
        id_handlers_[i].reset(new IdHandler());
      }
    }
  } else {
    for (int i = 0;
         i < static_cast<int>(SharedIdNamespaces::kNumSharedIdNamespaces);
         ++i) {
      if (i == static_cast<int>(SharedIdNamespaces::kProgramsAndShaders)) {
        id_handlers_[i].reset(new NonReusedIdHandler());
      } else {
        id_handlers_[i].reset(new StrictIdHandler(i));
      }
    }
  }
  program_info_manager_.reset(new ProgramInfoManager);
  for (auto& range_id_handler : range_id_handlers_) {
    range_id_handler.reset(new RangeIdHandler());
  }
}

void ShareGroup::Lose() {
  base::AutoLock hold(lost_lock_);
  lost_ = true;
}

bool ShareGroup::IsLost() const {
  base::AutoLock hold(lost_lock_);
  return lost_;
}

void ShareGroup::SetProgramInfoManagerForTesting(ProgramInfoManager* manager) {
  program_info_manager_.reset(manager);
}

ShareGroup::~ShareGroup() = default;

}  // namespace gles2
}  // namespace gpu
