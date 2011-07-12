// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_INPUT_EVENT_API_H_
#define PPAPI_THUNK_PPB_INPUT_EVENT_API_H_

#include "ppapi/c/ppb_input_event.h"

namespace ppapi {

struct InputEventData;

namespace thunk {

class PPB_InputEvent_API {
 public:
  virtual ~PPB_InputEvent_API() {}

  // This function is not exposed through the C API, but returns the internal
  // event data for easy proxying.
  virtual const InputEventData& GetInputEventData() const = 0;

  virtual PP_InputEvent_Type GetType() = 0;
  virtual PP_TimeTicks GetTimeStamp() = 0;
  virtual uint32_t GetModifiers() = 0;
  virtual PP_InputEvent_MouseButton GetMouseButton() = 0;
  virtual PP_Point GetMousePosition() = 0;
  virtual int32_t GetMouseClickCount() = 0;
  virtual PP_FloatPoint GetWheelDelta() = 0;
  virtual PP_FloatPoint GetWheelTicks() = 0;
  virtual PP_Bool GetWheelScrollByPage() = 0;
  virtual uint32_t GetKeyCode() = 0;
  virtual PP_Var GetCharacterText() = 0;
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_INPUT_EVENT_API_H_
