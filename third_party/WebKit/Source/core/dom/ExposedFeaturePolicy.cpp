#include "core/dom/ExposedFeaturePolicy.h"
#include <iostream>

namespace blink {
class ExposedFeaturePolicyIterationSource final

    : public PairIterable<String, bool>::IterationSource {
  bool next(ScriptState* scriptState,
            String& key,
            bool& value,
            ExceptionState&) override {
    return false;
  }
};
// static
ExposedFeaturePolicy* ExposedFeaturePolicy::create() {
  return new ExposedFeaturePolicy();
}

bool ExposedFeaturePolicy::enabled() {
  return true;
}

bool ExposedFeaturePolicy::getMapEntry(
    ScriptState* scriptState,
    const String& key,
    //                   v8::Local<v8::Value>& value,
    bool& value,
    ExceptionState& exceptionState) {
  // v8::Isolate* isolate = scriptState->isolate();
  // value = v8::undefined(isolate);
  std::cout << key.utf8().data() << std::endl;
  if (key == "midi") {
    value = true;
    return true;
  }
  if (key == "fullscreen") {
    value = false;
    return true;
  }
  return false;  // not found
}

PairIterable<String, bool>::IterationSource*
ExposedFeaturePolicy::startIteration(ScriptState*, ExceptionState&) {
  return new ExposedFeaturePolicyIterationSource(/*this*/);
}

DEFINE_TRACE(ExposedFeaturePolicy) {}

}  // namespace blink
