// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/locks/lock_manager.h"

#include <algorithm>
#include <utility>

#include "base/stl_util.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

namespace {

template <typename T>
bool Intersects(const std::vector<T>& a, const std::unordered_set<T>& b) {
  for (const auto& k : a) {
    if (b.find(k) != b.end())
      return true;
  }
  return false;
}

// A LockHandle is passed to the client when a lock is granted. As long as the
// handle is held, the lock is held. Dropping the handle - either explicitly
// by script or by process termination - causes the lock to be released.
class LockHandleImpl final : public blink::mojom::LockHandle {
 public:
  static blink::mojom::LockHandlePtr Create(base::WeakPtr<LockManager> context,
                                            const url::Origin& origin,
                                            int64_t id) {
    blink::mojom::LockHandlePtr ptr;
    mojo::MakeStrongBinding(
        base::MakeUnique<LockHandleImpl>(std::move(context), origin, id),
        mojo::MakeRequest(&ptr));
    return ptr;
  }

  LockHandleImpl(base::WeakPtr<LockManager> context,
                 const url::Origin& origin,
                 int64_t id)
      : context_(context), origin_(origin), id_(id) {}

  ~LockHandleImpl() override {
    if (context_)
      context_->ReleaseLock(origin_, id_);
  }

 private:
  base::WeakPtr<LockManager> context_;
  const url::Origin origin_;
  const int64_t id_;

  DISALLOW_COPY_AND_ASSIGN(LockHandleImpl);
};

}  // namespace

// A requested or held lock. When granted, a LockHandle will be minted
// and passed to the held callback. Eventually the client will drop the
// handle, which will notify the context and remove this.
struct LockManager::Lock {
  Lock(std::vector<std::string> scope,
       LockMode mode,
       int64_t id,
       blink::mojom::LockRequestPtr request)
      : scope(std::move(scope)),
        mode(mode),
        id(id),
        request(std::move(request)) {}

  ~Lock() = default;

  const std::vector<std::string> scope;
  const LockMode mode;
  const int64_t id;
  blink::mojom::LockRequestPtr request;
};

LockManager::LockManager() : weak_ptr_factory_(this) {}

LockManager::~LockManager() = default;

LockManager::OriginState::OriginState() = default;
LockManager::OriginState::~OriginState() = default;

void LockManager::CreateService(blink::mojom::LockManagerRequest request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bindings_.AddBinding(this, std::move(request));
}

void LockManager::RequestLock(const url::Origin& origin,
                              const std::vector<std::string>& scope,
                              LockMode mode,
                              WaitMode wait,
                              blink::mojom::LockRequestPtr request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (wait == WaitMode::NO_WAIT && !IsGrantable(origin, scope, mode)) {
    request->Failed();
    return;
  }

  int64_t lock_id = next_lock_id++;

  request.set_connection_error_handler(base::BindOnce(
      &LockManager::ReleaseLock, base::Unretained(this), origin, lock_id));

  origins_[origin].requested.emplace(std::make_pair(
      lock_id, std::make_unique<Lock>(std::move(scope), mode, lock_id,
                                      std::move(request))));

  ProcessRequests(origin);
}

void LockManager::ReleaseLock(const url::Origin& origin, int64_t id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!base::ContainsKey(origins_, origin))
    return;
  OriginState& state = origins_[origin];

  bool dirty = state.requested.erase(id) || state.held.erase(id);

  if (state.requested.empty() && state.held.empty())
    origins_.erase(origin);
  else if (dirty)
    ProcessRequests(origin);
}

bool LockManager::IsGrantable(const url::Origin& origin,
                              const std::vector<std::string>& scope,
                              LockMode mode) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!base::ContainsKey(origins_, origin))
    return true;

  OriginState& state = origins_[origin];

  if (mode == LockMode::EXCLUSIVE) {
    return !Intersects(scope, state.shared) &&
           !Intersects(scope, state.exclusive);
  } else {
    return !Intersects(scope, state.exclusive);
  }
}

void LockManager::ProcessRequests(const url::Origin& origin) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!base::ContainsKey(origins_, origin))
    return;

  OriginState& state = origins_[origin];

  if (state.requested.empty())
    return;

  state.shared.clear();
  state.exclusive.clear();
  for (const auto& id_lock_pair : state.held) {
    const auto& lock = id_lock_pair.second;
    (lock->mode == LockMode::SHARED ? state.shared : state.exclusive)
        .insert(lock->scope.begin(), lock->scope.end());
  }

  for (auto it = state.requested.begin(); it != state.requested.end();) {
    auto& lock = it->second;
    bool granted = false;

    if (lock->mode == LockMode::EXCLUSIVE) {
      if (!Intersects(lock->scope, state.shared) &&
          !Intersects(lock->scope, state.exclusive)) {
        granted = true;
      }
    } else {
      if (!Intersects(lock->scope, state.exclusive))
        granted = true;
    }

    (lock->mode == LockMode::SHARED ? state.shared : state.exclusive)
        .insert(lock->scope.begin(), lock->scope.end());

    if (granted) {
      std::unique_ptr<Lock> grantee = std::move(lock);
      it = state.requested.erase(it);
      grantee->request->Granted(LockHandleImpl::Create(
          weak_ptr_factory_.GetWeakPtr(), origin, grantee->id));
      grantee->request = nullptr;
      state.held.insert(std::make_pair(grantee->id, std::move(grantee)));
    } else {
      ++it;
    }
  }

  DCHECK(!(state.requested.empty() && state.held.empty()));
}

}  // namespace content
