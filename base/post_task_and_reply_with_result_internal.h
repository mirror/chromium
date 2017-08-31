// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_POST_TASK_AND_REPLY_WITH_RESULT_INTERNAL_H_
#define BASE_POST_TASK_AND_REPLY_WITH_RESULT_INTERNAL_H_

#include <tuple>
#include <utility>

#include "base/callback.h"
#include "base/optional.h"
#include "base/tuple.h"

namespace base {

namespace internal {

// Adapts a function that produces a result via a return value to
// one that returns via an output parameter.
template <typename ReturnType>
void ReturnAsParamAdapter(OnceCallback<ReturnType()> func, ReturnType* result) {
  *result = std::move(func).Run();
}

// Adapts a T* result to a callblack that expects a T.
template <typename TaskReturnType, typename ReplyArgType>
void ReplyAdapter(OnceCallback<void(ReplyArgType)> callback,
                  TaskReturnType* result) {
  std::move(callback).Run(std::move(*result));
}

// Adapts an asynchronous task, which takes a callback with returning args,
// to a task which takes a callback without args but instead it stores the
// result values in |result|.
template <typename ReturnTuple, typename... ReturnArgTypes>
void AsyncTaskAdaptor(
    OnceCallback<void(OnceCallback<void(ReturnArgTypes...)>)> task,
    base::Optional<ReturnTuple>* result,
    OnceClosure on_complete) {
  std::move(task).Run(BindOnce(
      [](base::Optional<ReturnTuple>* result, OnceClosure on_complete,
         ReturnArgTypes... args) {
        result->emplace(std::forward<ReturnArgTypes>(args)...);
        std::move(on_complete).Run();
      },
      Unretained(result), std::move(on_complete)));
}

// TODO: Maybe move this to other file, like base/tuple.h.
template <typename Tuple, typename... ArgTypes, size_t... Is>
void CallbackApply(OnceCallback<void(ArgTypes...)> callback,
                   Tuple&& args,
                   std::index_sequence<Is...>) {
  std::move(callback).Run(std::get<Is>(std::forward<Tuple>(args))...);
}

// TODO: Merge this and ReplyAdapter.
template <typename ReturnTuple, typename... ReplyArgTypes>
void ReplyAsyncAdaptor(OnceCallback<void(ReplyArgTypes...)> reply,
                       base::Optional<ReturnTuple>* result) {
  CallbackApply(std::move(reply), std::move(result->value()),
                MakeIndexSequenceForTuple<ReturnTuple>());
}

}  // namespace internal

}  // namespace base

#endif  // BASE_POST_TASK_AND_REPLY_WITH_RESULT_INTERNAL_H_
