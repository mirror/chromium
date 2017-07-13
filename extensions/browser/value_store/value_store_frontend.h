// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_VALUE_STORE_VALUE_STORE_FRONTEND_H_
#define EXTENSIONS_BROWSER_VALUE_STORE_VALUE_STORE_FRONTEND_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/values.h"

namespace extensions {
class ValueStoreFactory;
}

// A frontend for a LeveldbValueStore, for use on the UI thread.
class ValueStoreFrontend {
 public:
  // The kind of extensions data stored in a backend.
  enum class BackendType { RULES, STATE };

  typedef base::OnceCallback<void(std::unique_ptr<base::Value>)> ReadCallback;

  ValueStoreFrontend(
      const scoped_refptr<extensions::ValueStoreFactory>& store_factory,
      BackendType backend_type);
  ~ValueStoreFrontend();

  // Retrieves a value from the database asynchronously, passing a copy to
  // |callback| when ready. NULL is passed if no matching entry is found.
  void Get(const std::string& key, ReadCallback callback);

  // Sets a value with the given key.
  void Set(const std::string& key, std::unique_ptr<base::Value> value);

  // Removes the value with the given key.
  void Remove(const std::string& key);

 private:
  class Backend;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // A helper class to manage lifetime of the backing ValueStore, which lives
  // on the |task_runner_|.
  scoped_refptr<Backend> backend_;

  DISALLOW_COPY_AND_ASSIGN(ValueStoreFrontend);
};

#endif  // EXTENSIONS_BROWSER_VALUE_STORE_VALUE_STORE_FRONTEND_H_
