// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/flags/flags_context.h"

#include <algorithm>
#include <utility>

#include "base/stl_util.h"
#include "content/public/browser/browser_thread.h"

namespace content {

// TODO:
// * Convert Flag over to an mojo interface so that it can
//   be cleaned up automagically when the other end goes away.
// * Consider dealing with "closing swarms" and defer processing.
// * Connection errors!

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
}  // namespace

struct FlagsContext::Flag {
  Flag(const std::set<std::string>& scope,
       blink::mojom::FlagMode mode,
       int64_t id,
       blink::mojom::FlagsService::RequestFlagCallback callback)
      : scope(scope), mode(mode), id(id), callback(std::move(callback)) {}

  ~Flag() {}

  const std::set<std::string> scope;
  const blink::mojom::FlagMode mode;
  const int64_t id;
  blink::mojom::FlagsService::RequestFlagCallback callback;
};

FlagsContext::FlagsContext() {}

FlagsContext::~FlagsContext() {}

void FlagsContext::CreateService(blink::mojom::FlagsServiceRequest request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bindings_.AddBinding(this, std::move(request));
}

void FlagsContext::RequestFlag(
    const url::Origin& origin,
    const std::vector<std::string>& scope,
    blink::mojom::FlagMode mode,
    blink::mojom::FlagsService::RequestFlagCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  int64_t flag_id = next_id_++;
  flags_requested_[origin].emplace_back(
      std::make_unique<Flag>(std::set<std::string>(scope.begin(), scope.end()),
                             mode, flag_id, std::move(callback)));
  ProcessRequests(origin);
}

void FlagsContext::ReleaseFlag(const url::Origin& origin, int64_t id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (base::ContainsKey(flags_requested_, origin)) {
    auto& requested = flags_requested_[origin];
    const auto& it =
        std::find_if(requested.begin(), requested.end(),
                     [id](const std::unique_ptr<Flag>& flag) -> bool {
                       return flag->id == id;
                     });
    if (it != requested.end()) {
      requested.erase(it);
      if (requested.empty())
        flags_requested_.erase(origin);
      return;
    }
  }

  if (base::ContainsKey(flags_held_, origin)) {
    auto& held = flags_held_[origin];
    const auto& it =
        std::find_if(held.begin(), held.end(),
                     [id](const std::unique_ptr<Flag>& flag) -> bool {
                       return flag->id == id;
                     });
    if (it != held.end()) {
      held.erase(it);
      if (held.empty())
        flags_held_.erase(origin);
      ProcessRequests(origin);
      return;
    }
  }
}

void FlagsContext::ProcessRequests(const url::Origin& origin) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!base::ContainsKey(flags_requested_, origin))
    return;

  std::set<std::string> shared, exclusive;
  if (base::ContainsKey(flags_held_, origin)) {
    for (const auto& flag : flags_held_[origin]) {
      (flag->mode == blink::mojom::FlagMode::SHARED ? shared : exclusive)
          .insert(flag->scope.begin(), flag->scope.end());
    }
  }

  auto& requested = flags_requested_[origin];
  for (auto it = requested.begin(); it != requested.end();) {
    auto& flag = *it;

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
      it = requested.erase(it);
      std::move(grantee->callback).Run(grantee->id);
      flags_held_[origin].push_back(std::move(grantee));
    } else {
      ++it;
    }
  }

  if (base::ContainsKey(flags_requested_, origin) &&
      flags_requested_[origin].empty())
    flags_requested_.erase(origin);
}

}  // namespace content
