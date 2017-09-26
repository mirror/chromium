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

// TODO(jsbell): Timeouts.

namespace {

// Intersection test for two std::set<> instances. Relies on the
// implicit ordered nature of std::set.
template <typename T>
bool Intersects(const std::set<T>& a, const std::set<T>& b) {
  if (a.empty() || b.empty())
    return false;

  auto iter_a = a.begin();
  auto iter_b = b.begin();
  if (*iter_a > *b.rbegin() || *iter_b > *a.rbegin())
    return false;

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
  Lock(const std::set<std::string>& scope,
       blink::mojom::LockMode mode,
       int64_t id,
       blink::mojom::LockRequestPtr request)
      : scope(scope), mode(mode), id(id), request(std::move(request)) {}

  ~Lock() = default;

  const std::set<std::string> scope;
  const blink::mojom::LockMode mode;
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
                               blink::mojom::LockMode mode,
                               blink::mojom::LockRequestPtr request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  int64_t lock_id = next_id_++;

  request.set_connection_error_handler(
      base::BindOnce(&LocksContext::ReleaseLock, base::Unretained(this), origin,
                     lock_id, kDoProcessRequests));

  origins_[origin].requested.emplace(std::make_pair(
      lock_id,
      std::make_unique<Lock>(std::set<std::string>(scope.begin(), scope.end()),
                             mode, lock_id, std::move(request))));

  ProcessRequests(origin);
  DebugDumpState();
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
  DebugDumpState();
}

void LocksContext::ProcessRequests(const url::Origin& origin) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!base::ContainsKey(origins_, origin))
    return;

  OriginState& state = origins_[origin];

  if (state.requested.empty())
    return;

  std::set<std::string> shared, exclusive;
  for (const auto& id_lock_pair : state.held) {
    const auto& lock = id_lock_pair.second;
    (lock->mode == blink::mojom::LockMode::SHARED ? shared : exclusive)
        .insert(lock->scope.begin(), lock->scope.end());
  }

  for (auto it = state.requested.begin(); it != state.requested.end();) {
    auto& lock = it->second;
    bool granted = false;

    if (lock->mode == blink::mojom::LockMode::EXCLUSIVE) {
      if (!Intersects(lock->scope, shared) &&
          !Intersects(lock->scope, exclusive)) {
        granted = true;
      }
      exclusive.insert(lock->scope.begin(), lock->scope.end());
    } else {
      if (!Intersects(lock->scope, exclusive))
        granted = true;
      shared.insert(lock->scope.begin(), lock->scope.end());
    }

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

void LocksContext::DebugDumpState() const {
  if (origins_.empty()) {
    DLOG(WARNING) << " -*- no held or pending locks -*- ";
    return;
  }

  for (const auto& pair : origins_) {
    auto& origin = pair.first;
    auto& state = pair.second;

    DLOG(WARNING) << "";
    DLOG(WARNING) << " == " << origin.Serialize() << " == ";

    auto dump = [](const std::map<int64_t, std::unique_ptr<Lock>>& map) {
      for (const auto& id_lock_pair : map) {
        const auto& lock = id_lock_pair.second;
        std::ostringstream oss;
        std::copy(lock->scope.begin(), lock->scope.end(),
                  std::ostream_iterator<std::string>(oss, ", "));
        DLOG(WARNING) << "    id: " << lock->id << "  mode: " << lock->mode
                      << "  scope: " << oss.str();
      }
    };

    DLOG(WARNING) << "  held: ";
    dump(state.held);

    DLOG(WARNING) << "  requested: ";
    dump(state.requested);
  }
}

}  // namespace content
