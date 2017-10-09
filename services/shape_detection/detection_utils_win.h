// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef SERVICES_SHAPE_DETECTION_DETECTION_UTILS_WIN_H_
#define SERVICES_SHAPE_DETECTION_DETECTION_UTILS_WIN_H_

#include "base/threading/thread.h"

namespace shape_detection {

using namespace ABI::Windows::Foundation;

template <typename RuntimeType, class T>
class AsyncOperationTemplate {
 public:
  AsyncOperationTemplate(T* detection_impl) : detection_impl_(detection_impl) {}

  virtual ~AsyncOperationTemplate() {}

  void asyncOperate(IAsyncOperation<RuntimeType*>* async_op) {
    base::WeakPtr<AsyncOperationTemplate> weak_ptr = GetWeakPtrFromFactory();
    scoped_refptr<base::SingleThreadTaskRunner> task_runner =
        base::ThreadTaskRunnerHandle::Get();

    auto asyncCallback = WRL::Callback<WRL::Implements<
        WRL::RuntimeClassFlags<WRL::ClassicCom>,
        IAsyncOperationCompletedHandler<RuntimeType*>, WRL::FtmBase>>(
        [weak_ptr, task_runner](IAsyncOperation<RuntimeType*>* async_op,
                                AsyncStatus status) {
          DVLOG(1) << "--inside CreateAsync() async handler";
          // A reference to |async_op| is kept in |async_ops_|, safe to pass
          // outside.  This is happening on an OS thread.
          task_runner->PostTask(
              FROM_HERE,
              base::Bind(&AsyncOperationTemplate::asyncCallbackInternal,
                         weak_ptr, async_op));

          return S_OK;
        });

    HRESULT hr = async_op->put_Completed(asyncCallback.Get());
    if (FAILED(hr)) {
      DLOG(ERROR) << "put_Completed() failed: " << PrintHr(hr) << ", "
                  << PrintHr(HRESULT_FROM_WIN32(GetLastError()));
      return;
    }

    // Keep a reference to incompleted |async_op| for releasing later.
    async_ops_.insert(async_op);
  }

 protected:
  T* detection_impl_;

 private:
  void asyncCallbackInternal(IAsyncOperation<RuntimeType*>* async_op) {
    asyncCallback(async_op);

    // Manually release COM interface to completed |async_op|.
    auto it = async_ops_.find(async_op);
    CHECK(it != async_ops_.end());
    (*it)->Release();
    async_ops_.erase(it);
  }
  // WeakPtrFactory has to be declared in derived class, use this method to
  // retrieve upcasted WeakPtr for posting tasks.
  virtual base::WeakPtr<AsyncOperationTemplate> GetWeakPtrFromFactory() = 0;

  virtual void asyncCallback(IAsyncOperation<RuntimeType*>* async_op) = 0;

  // Keeps AsyncOperation references before the operation completes. Note that
  // raw pointers are used here and the COM interfaces should be released
  // manually.
  std::unordered_set<IAsyncOperation<RuntimeType*>*> async_ops_;
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_DETECTION_UTILS_WIN_H_