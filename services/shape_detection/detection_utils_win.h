// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SHAPE_DETECTION_DETECTION_UTILS_WIN_H_
#define SERVICES_SHAPE_DETECTION_DETECTION_UTILS_WIN_H_

#include <wrl\event.h>
#include <wrl\implements.h>

#include "base/threading/thread.h"

using ABI::Windows::Foundation::IAsyncOperation;
using ABI::Windows::Foundation::IAsyncOperationCompletedHandler;

namespace shape_detection {

namespace WRL = Microsoft::WRL;

// A class represents an asynchronous operation which returns a result
// upon completion. RuntimeType is Windows Runtime APIs that have a result.
template <typename RuntimeType>
class AsyncOperation {
 public:
  using IAsyncOperationPtr =
      Microsoft::WRL::ComPtr<IAsyncOperation<RuntimeType*>>;
  using Callback = base::OnceCallback<void(IAsyncOperationPtr)>;

  explicit AsyncOperation(Callback callback, IAsyncOperationPtr async_op_ptr)
      : async_op_ptr_(std::move(async_op_ptr)),
        callback_(std::move(callback)),
        weak_factory_(this) {}

  ~AsyncOperation() = default;

  // Sets the method that is called when the asynchronous action completes.
  HRESULT PutCompleted() {
    base::WeakPtr<AsyncOperation> weak_ptr = weak_factory_.GetWeakPtr();
    scoped_refptr<base::SingleThreadTaskRunner> task_runner =
        base::ThreadTaskRunnerHandle::Get();

    typedef WRL::Implements<WRL::RuntimeClassFlags<WRL::ClassicCom>,
                            IAsyncOperationCompletedHandler<RuntimeType*>,
                            WRL::FtmBase>
        AsyncCallback;
    auto asyncCallback = WRL::Callback<AsyncCallback>(
        [weak_ptr, task_runner](IAsyncOperation<RuntimeType*>* async_op,
                                AsyncStatus status) {
          // A reference to |async_op| is kept in |async_op_ptr_|, safe to pass
          // outside.  This is happening on an OS thread.
          task_runner->PostTask(
              FROM_HERE, base::Bind(&AsyncOperation::AsyncCallbackInternal,
                                    std::move(weak_ptr), async_op));

          return S_OK;
        });

    return async_op_ptr_->put_Completed(asyncCallback.Get());
  }

 private:
  void AsyncCallbackInternal(IAsyncOperation<RuntimeType*>* async_op) {
    std::move(callback_).Run(std::move(async_op_ptr_));
  }

  IAsyncOperationPtr async_op_ptr_;
  Callback callback_;

  base::WeakPtrFactory<AsyncOperation<RuntimeType>> weak_factory_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(AsyncOperation);
};

}  // namespace shape_detection

#endif  // SERVICES_SHAPE_DETECTION_DETECTION_UTILS_WIN_H_