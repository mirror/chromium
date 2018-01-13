// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_SESSION_H_
#define CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_SESSION_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"

namespace base {
class SequencedTaskRunner;
}

namespace content {

class DOMStorageContextImpl;
class DOMStorageContextWrapper;

// This refcounted class determines the lifetime of a session
// storage namespace and provides an interface to Clone() an
// existing session storage namespace. It may be used on any thread.
// See class comments for DOMStorageContextImpl for a larger overview.
class CONTENT_EXPORT DOMStorageSession {
 public:
  // Constructs a |DOMStorageSession| and allocates new IDs for it.
  explicit DOMStorageSession(scoped_refptr<DOMStorageContextImpl> context);

  // Constructs a |DOMStorageSession| for the mojo version of session storage
  // and allocates new IDs for it.
  explicit DOMStorageSession(scoped_refptr<DOMStorageContextWrapper> context);

  // Constructs a |DOMStorageSession| and assigns |persistent_namespace_id|
  // to it. Allocates a new non-persistent ID.
  DOMStorageSession(scoped_refptr<DOMStorageContextImpl> context,
                    const std::string& persistent_namespace_id);

  // Constructs a |DOMStorageSession| for the mojo version of session storage
  // and assigns |persistent_namespace_id| to it. Allocates a new
  // non-persistent ID.
  DOMStorageSession(scoped_refptr<DOMStorageContextWrapper> context,
                    const std::string& persistent_namespace_id);

  ~DOMStorageSession();

  int64_t namespace_id() const { return namespace_id_; }
  const std::string& persistent_namespace_id() const {
    return persistent_namespace_id_;
  }
  void SetShouldPersist(bool should_persist);
  bool should_persist() const;
  bool IsFromContext(DOMStorageContextImpl* context);

  std::unique_ptr<DOMStorageSession> Clone();

  // Constructs a |DOMStorageSession| by cloning
  // |namespace_id_to_clone|. Allocates new IDs for it.
  static std::unique_ptr<DOMStorageSession> CloneFrom(
      scoped_refptr<DOMStorageContextImpl>,
      int64_t namepace_id_to_clone);

  // Constructs a |DOMStorageSession| backed by mojo by cloning
  // |namespace_id_to_clone|. Allocates new IDs for it.
  static std::unique_ptr<DOMStorageSession> CloneFrom(
      scoped_refptr<DOMStorageContextWrapper>,
      int64_t namepace_id_to_clone);

 private:
  class SequenceHelper;

  DOMStorageSession(scoped_refptr<DOMStorageContextImpl> context,
                    int64_t namespace_id,
                    const std::string& persistent_namespace_id);
  DOMStorageSession(scoped_refptr<DOMStorageContextWrapper> context,
                    int64_t namespace_id,
                    const std::string& persistent_namespace_id);

  scoped_refptr<DOMStorageContextImpl> context_;
  scoped_refptr<DOMStorageContextWrapper> mojo_context_;
  scoped_refptr<base::SequencedTaskRunner> mojo_task_runner_;
  int64_t namespace_id_;
  std::string persistent_namespace_id_;
  bool should_persist_;

  // Contructed on constructing thread of DOMStorageSession, used and destroyed
  // on |mojo_task_runner_|.
  std::unique_ptr<SequenceHelper> sequence_helper_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(DOMStorageSession);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_SESSION_H_
