// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/flags/flags_context.h"

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

// A FlagHandle is passed to the client when a flag is granted. As long as the
// handle is held, the flag is held. Dropping the handle - either explicitly
// by script or by process termination - causes the flag to be released.
class FlagHandleImpl final : public blink::mojom::FlagHandle {
 public:
  static blink::mojom::FlagHandlePtr Create(base::WeakPtr<FlagsContext> context,
                                            const url::Origin& origin,
                                            int64_t id) {
    blink::mojom::FlagHandlePtr ptr;
    mojo::MakeStrongBinding(
        base::MakeUnique<FlagHandleImpl>(std::move(context), origin, id),
        mojo::MakeRequest(&ptr));
    return ptr;
  }

  FlagHandleImpl(base::WeakPtr<FlagsContext> context,
                 const url::Origin& origin,
                 int64_t id)
      : context_(context), origin_(origin), id_(id) {}

  ~FlagHandleImpl() override {
    if (context_)
      context_->ReleaseFlag(origin_, id_);
  }

 private:
  base::WeakPtr<FlagsContext> context_;
  const url::Origin origin_;
  const int64_t id_;

  DISALLOW_COPY_AND_ASSIGN(FlagHandleImpl);
};

}  // namespace

// A requested or held flag. When granted, a FlagHandle will be minted
// and passed to the held callback. Eventually the client will drop the
// handle, which will notify the context and remove this.
struct FlagsContext::Flag {
  Flag(const std::set<std::string>& scope,
       blink::mojom::FlagMode mode,
       int64_t id,
       blink::mojom::FlagsService::RequestFlagCallback callback)
      : scope(scope), mode(mode), id(id), callback(std::move(callback)) {}

  ~Flag() = default;

  const std::set<std::string> scope;
  const blink::mojom::FlagMode mode;
  const int64_t id;
  blink::mojom::FlagsService::RequestFlagCallback callback;
};

// An instance of this is associated (via the BindingSet) with each binding;
// it tracks the requests that have not yet been services so they can be
// terminated if the binding dies.
class FlagsContext::BindingContext {
 public:
  explicit BindingContext(FlagsContext* context) : context_(context) {}

  ~BindingContext() {
    // Cancel all pending requests, deferring processing of pending requests
    // to prevent churn and re-entrancy.
    std::set<url::Origin> origins;
    for (const auto& pair : pending_requests_) {
      const url::Origin& origin = pair.first;
      int64_t id = pair.second;
      origins.insert(origin);
      context_->ReleaseFlag(origin, id, kDoNotProcessRequests);
    }
    // Now go back and process the dirtied origins.
    for (const auto& origin : origins)
      context_->ProcessRequests(origin);
  }

  void AddPendingRequest(const url::Origin origin, int64_t id) {
    pending_requests_.insert(std::make_pair(origin, id));
  }

  void RemovePendingRequest(const url::Origin origin, int64_t id) {
    pending_requests_.erase(std::make_pair(origin, id));
  }

 private:
  // Owns this, so raw pointer is safe.
  FlagsContext* context_;

  std::set<std::pair<url::Origin, int64_t>> pending_requests_;
};

FlagsContext::FlagsContext() : weak_ptr_factory_(this) {}

FlagsContext::~FlagsContext() = default;

FlagsContext::OriginState::OriginState() = default;
FlagsContext::OriginState::~OriginState() = default;

void FlagsContext::CreateService(blink::mojom::FlagsServiceRequest request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bindings_.AddBinding(this, std::move(request),
                       base::MakeUnique<BindingContext>(this));
}

void FlagsContext::RequestFlag(
    const url::Origin& origin,
    const std::vector<std::string>& scope,
    blink::mojom::FlagMode mode,
    blink::mojom::FlagsService::RequestFlagCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  int64_t flag_id = next_id_++;

  bindings_.dispatch_context()->AddPendingRequest(origin, flag_id);

  auto remove_pending_request = base::BindOnce(
      &BindingContext::RemovePendingRequest,
      base::Unretained(bindings_.dispatch_context().get()), origin, flag_id);

  origins_[origin].requested.emplace(std::make_pair(
      flag_id,
      std::make_unique<Flag>(
          std::set<std::string>(scope.begin(), scope.end()), mode, flag_id,
          base::BindOnce(
              [](base::OnceClosure remove_pending_request,
                 blink::mojom::FlagsService::RequestFlagCallback callback,
                 blink::mojom::FlagHandlePtr handle) {
                // When the flag request is granted, remove the pending request
                // entry from the binding context.
                std::move(remove_pending_request).Run();
                // And then actually service the request.
                std::move(callback).Run(std::move(handle));
              },
              std::move(remove_pending_request), std::move(callback)))));
  ProcessRequests(origin);
  DebugDumpState();
}

void FlagsContext::ReleaseFlag(const url::Origin& origin,
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

void FlagsContext::ProcessRequests(const url::Origin& origin) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!base::ContainsKey(origins_, origin))
    return;

  OriginState& state = origins_[origin];

  if (state.requested.empty())
    return;

  std::set<std::string> shared, exclusive;
  for (const auto& id_flag_pair : state.held) {
    const auto& flag = id_flag_pair.second;
    (flag->mode == blink::mojom::FlagMode::SHARED ? shared : exclusive)
        .insert(flag->scope.begin(), flag->scope.end());
  }

  for (auto it = state.requested.begin(); it != state.requested.end();) {
    auto& flag = it->second;
    bool granted = false;

    if (flag->mode == blink::mojom::FlagMode::EXCLUSIVE) {
      if (!Intersects(flag->scope, shared) &&
          !Intersects(flag->scope, exclusive)) {
        granted = true;
      }
      exclusive.insert(flag->scope.begin(), flag->scope.end());
    } else {
      if (!Intersects(flag->scope, exclusive))
        granted = true;
      shared.insert(flag->scope.begin(), flag->scope.end());
    }

    if (granted) {
      std::unique_ptr<Flag> grantee = std::move(flag);
      it = state.requested.erase(it);
      std::move(grantee->callback)
          .Run(FlagHandleImpl::Create(weak_ptr_factory_.GetWeakPtr(), origin,
                                      grantee->id));
      state.held.insert(std::make_pair(grantee->id, std::move(grantee)));
    } else {
      ++it;
    }
  }

  DCHECK(!(state.requested.empty() && state.held.empty()));
}

void FlagsContext::DebugDumpState() const {
  if (origins_.empty()) {
    DLOG(WARNING) << " -*- no held or pending flags -*- ";
    return;
  }

  for (const auto& pair : origins_) {
    auto& origin = pair.first;
    auto& state = pair.second;

    DLOG(WARNING) << "";
    DLOG(WARNING) << " == " << origin.Serialize() << " == ";

    auto dump = [](const std::map<int64_t, std::unique_ptr<Flag>>& map) {
      for (const auto& id_flag_pair : map) {
        const auto& flag = id_flag_pair.second;
        std::ostringstream oss;
        std::copy(flag->scope.begin(), flag->scope.end(),
                  std::ostream_iterator<std::string>(oss, ", "));
        DLOG(WARNING) << "    id: " << flag->id << "  mode: " << flag->mode
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
