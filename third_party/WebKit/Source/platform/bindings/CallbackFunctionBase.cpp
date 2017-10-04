#include "platform/bindings/CallbackFunctionBase.h"
#include "platform/bindings/ScriptState.h"

namespace blink {

CallbackFunctionBase::CallbackFunctionBase(ScriptState* scriptState,
                                           v8::Local<v8::Function> callback)
    : script_state_(scriptState),
      callback_(scriptState->GetIsolate(), this, callback) {
  DCHECK(!callback_.IsEmpty());
}

DEFINE_TRACE_WRAPPERS(CallbackFunctionBase) {
  visitor->TraceWrappers(callback_.Cast<v8::Value>());
}

}  // namespace blink
