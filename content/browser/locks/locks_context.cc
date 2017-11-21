// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/locks/locks_context.h"

#include <algorithm>
#include <utility>

#include "base/stl_util.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

namespace {

// Intersection test for two std::set<> instances.
template <typename T>
bool Intersects(const std::set<T>& a, const std::set<T>& b) {
  // Relies on the implicit ordered nature of std::set.
  if (a.empty() || b.empty())
    return false;

  auto iter_a = a.begin();
  auto iter_b = b.begin();
  // Early exit if the ranges do not overlap, as an optimization.
  if (*iter_a > *b.rbegin() || *iter_b > *a.rbegin())
    return false;

  // TODO(jsbell): If a.size() << b.size() (or vice versa), it will be more
  // efficient to iterate over the smaller set and do contains tests.

  while (iter_a != a.end() && iter_b != b.end()) {
    if (*iter_a == *iter_b)
      return true;
    if (*iter_a < *iter_b)
      ++iter_a;
    else
      ++iter_b;
  }
  return false;
}

// A LockHandle is passed to the client when a lock is granted. As long as the
// handle is held, the lock is held. Dropping the handle - either explicitly
// by script or by process termination - causes the lock to be released.
class LockHandleImpl final : public blink::mojom::LockHandle {
 public:
  static blink::mojom::LockHandlePtr Create(base::WeakPtr<LocksContext> context,
                                            const url::Origin& origin,
                                            int64_t id) {
    blink::mojom::LockHandlePtr ptr;
    mojo::MakeStrongBinding(
        base::MakeUnique<LockHandleImpl>(std::move(context), origin, id),
        mojo::MakeRequest(&ptr));
    return ptr;
  }

  LockHandleImpl(base::WeakPtr<LocksContext> context,
                 const url::Origin& origin,
                 int64_t id)
      : context_(context), origin_(origin), id_(id) {}

  ~LockHandleImpl() override {
    if (context_)
      context_->ReleaseLock(origin_, id_);
  }

 private:
  base::WeakPtr<LocksContext> context_;
  const url::Origin origin_;
  const int64_t id_;

  DISALLOW_COPY_AND_ASSIGN(LockHandleImpl);
};

}  // namespace

// A requested or held lock. When granted, a LockHandle will be minted
// and passed to the held callback. Eventually the client will drop the
// handle, which will notify the context and remove this.
struct LocksContext::Lock {
  Lock(std::set<std::string> scope,
       LockMode mode,
       int64_t id,
       blink::mojom::LockRequestPtr request)
      : scope(std::move(scope)),
        mode(mode),
        id(id),
        request(std::move(request)) {}

  ~Lock() = default;

  const std::set<std::string> scope;
  const LockMode mode;
  const int64_t id;
  blink::mojom::LockRequestPtr request;
};

LocksContext::LocksContext() : weak_ptr_factory_(this) {}

LocksContext::~LocksContext() = default;

LocksContext::OriginState::OriginState() = default;
LocksContext::OriginState::~OriginState() = default;

void LocksContext::CreateService(blink::mojom::LocksServiceRequest request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bindings_.AddBinding(this, std::move(request));
}

void LocksContext::RequestLock(const url::Origin& origin,
                               const std::vector<std::string>& scope,
                               LockMode mode,
                               WaitMode wait,
                               blink::mojom::LockRequestPtr request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::set<std::string> scope_set(scope.begin(), scope.end());

  if (wait == WaitMode::NO_WAIT && !IsGrantable(origin, scope_set, mode)) {
    request->Failed();
    return;
  }

  int64_t lock_id = next_lock_id++;

  request.set_connection_error_handler(
      base::BindOnce(&LocksContext::ReleaseLock, base::Unretained(this), origin,
                     lock_id, kDoProcessRequests));

  origins_[origin].requested.emplace(std::make_pair(
      lock_id, std::make_unique<Lock>(std::move(scope_set), mode, lock_id,
                                      std::move(request))));

  ProcessRequests(origin);
}

void LocksContext::ReleaseLock(const url::Origin& origin,
                               int64_t id,
                               PostReleaseAction action) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!base::ContainsKey(origins_, origin))
    return;
  OriginState& state = origins_[origin];

  bool dirty = state.requested.erase(id) || state.held.erase(id);

  if (state.requested.empty() && state.held.empty())
    origins_.erase(origin);
  else if (dirty && action == kDoProcessRequests)
    ProcessRequests(origin);
}

bool LocksContext::IsGrantable(const url::Origin& origin,
                               const std::set<std::string>& scope,
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

void LocksContext::ProcessRequests(const url::Origin& origin) {
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
