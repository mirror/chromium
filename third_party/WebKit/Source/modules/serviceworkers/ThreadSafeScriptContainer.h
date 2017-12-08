// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ThreadSafeScriptContainer_h
#define ThreadSafeScriptContainer_h

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "platform/network/HTTPHeaderMap.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/KURLHash.h"
#include "platform/wtf/HashMap.h"
#include "public/platform/WebString.h"
#include "public/platform/WebVector.h"

namespace blink {

// ThreadSafeScriptContainer stores the scripts of a service worker for
// startup. This container is created for each service worker. The IO thread
// adds scripts to the container, and the worker thread takes the scripts.
//
// This class uses explicit synchronization because it needs to support
// synchronous importScripts() from the worker thread.
//
// This class is RefCounted because there is no ordering guarantee of lifetime
// of its owners, i.e. ServiceWorkerInstalledScriptsManager and its
// Internal class. ServiceWorkerInstalledScriptsManager is destroyed earlier
// than Internal if the worker is terminated before all scripts are streamed,
// and Internal is destroyed earlier if all of scripts are received before
// finishing script evaluation.
class ThreadSafeScriptContainer final
    : public base::RefCountedThreadSafe<ThreadSafeScriptContainer> {
 public:
  using BytesChunk = WebVector<char>;
  // Container of a script. All the fields of this class need to be
  // cross-thread-transfer-safe.
  class RawScriptData {
   public:
    static std::unique_ptr<RawScriptData> Create(
        WebString encoding,
        WebVector<BytesChunk> script_text,
        WebVector<BytesChunk> meta_data);
    static std::unique_ptr<RawScriptData> CreateInvalidInstance();

    // Implementation of the destructor should be in the Blink side because only
    // Blink can know all of members.
    ~RawScriptData();

    void AddHeader(const WebString& key, const WebString& value);

    // Returns false if it fails to receive the script from the browser.
    bool IsValid() const { return is_valid_; }
    // The encoding of the script text.
    const WebString& Encoding() const {
      DCHECK(is_valid_);
      return encoding_;
    }
    // An array of raw byte chunks of the script text.
    const WebVector<BytesChunk>& ScriptTextChunks() const {
      DCHECK(is_valid_);
      return script_text_;
    }
    // An array of raw byte chunks of the scripts's meta data from the script's
    // V8 code cache.
    const WebVector<BytesChunk>& MetaDataChunks() const {
      DCHECK(is_valid_);
      return meta_data_;
    }

    // The HTTP headers of the script.
    std::unique_ptr<CrossThreadHTTPHeaderMapData> TakeHeaders() {
      DCHECK(is_valid_);
      return std::move(headers_);
    }

   private:
    // This instance must be created on the Blink side because only Blink can
    // know the exact size of this instance.
    RawScriptData(WebString encoding,
                  WebVector<BytesChunk> script_text,
                  WebVector<BytesChunk> meta_data,
                  bool is_valid);
    const bool is_valid_;
    WebString encoding_;
    WebVector<BytesChunk> script_text_;
    WebVector<BytesChunk> meta_data_;
    std::unique_ptr<CrossThreadHTTPHeaderMapData> headers_;
  };

  REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE();
  ThreadSafeScriptContainer();

  enum class ScriptStatus {
    // The script data has been received.
    kReceived,
    // The script data has been received but it has already been taken.
    kTaken,
    // Receiving the script has failed.
    kFailed,
    // The script data has not been received yet.
    kPending
  };

  // Called on the IO thread.
  void AddOnIOThread(const KURL& script_url,
                     std::unique_ptr<RawScriptData> raw_data);

  // Called on the worker thread.
  ScriptStatus GetStatusOnWorkerThread(const KURL& script_url);

  // Removes the script. After calling this, ScriptStatus for the
  // script will be kPending.
  // Called on the worker thread.
  void ResetOnWorkerThread(const KURL& script_url);

  // Waits until the script is added. The thread is blocked until the script is
  // available or receiving the script fails. Returns false if an error happens
  // and the waiting script won't be available forever.
  // Called on the worker thread.
  bool WaitOnWorkerThread(const KURL& script_url);

  // Called on the worker thread.
  std::unique_ptr<RawScriptData> TakeOnWorkerThread(const KURL& script_url);

  // Called if no more data will be added.
  // Called on the IO thread.
  void OnAllDataAddedOnIOThread();

 private:
  friend class base::RefCountedThreadSafe<ThreadSafeScriptContainer>;
  ~ThreadSafeScriptContainer();

  // |lock_| protects |waiting_cv_|, |script_data_|, |waiting_url_| and
  // |are_all_data_added_|.
  base::Lock lock_;
  // |waiting_cv_| is signaled when a script whose url matches to |waiting_url|
  // is added, or OnAllDataAdded is called. The worker thread waits on this, and
  // the IO thread signals it.
  base::ConditionVariable waiting_cv_;
  WTF::HashMap<KURL, std::unique_ptr<RawScriptData>> script_data_;
  KURL waiting_url_;
  bool are_all_data_added_;
};

}  // namespace blink

#endif  // ThreadSafeScriptContainer_h
