#ifndef CallbackFunctionBase_h
#define CallbackFunctionBase_h

#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/TraceWrapperV8Reference.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class ScriptState;

class PLATFORM_EXPORT CallbackFunctionBase
    : public GarbageCollectedFinalized<CallbackFunctionBase>,
      public TraceWrapperBase {
 public:
  virtual ~CallbackFunctionBase() = default;

  DEFINE_INLINE_TRACE() {}
  DECLARE_TRACE_WRAPPERS();

  v8::Local<v8::Function> v8Value(v8::Isolate* isolate) {
    return callback_.NewLocal(isolate);
  }

 protected:
  CallbackFunctionBase(ScriptState*, v8::Local<v8::Function>);
  RefPtr<ScriptState> script_state_;
  TraceWrapperV8Reference<v8::Function> callback_;
};
}  // namespace blink

#endif  // CallbackFunctionBase_h
