#ifndef ExposedFeaturePolicy_h
#define ExposedFeaturePolicy_h

#include "bindings/core/v8/ActiveScriptWrappable.h"
#include "bindings/core/v8/Iterable.h"
#include "bindings/core/v8/Maplike.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptPromiseProperty.h"
#include "bindings/core/v8/ScriptWrappable.h"
#include "core/dom/ContextLifecycleObserver.h"
#include "core/dom/DOMException.h"
#include "platform/heap/GarbageCollected.h"
#include "wtf/text/WTFString.h"

namespace blink {

class CORE_EXPORT ExposedFeaturePolicy final
    : public GarbageCollectedFinalized<ExposedFeaturePolicy>,
      public ScriptWrappable,
      public Maplike<String, bool> {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static ExposedFeaturePolicy* create();
  bool enabled();
  bool getMapEntry(ScriptState*,
                   const String& key,
                   //                   v8::Local<v8::Value>&,
                   bool&,
                   ExceptionState&) override;
  DECLARE_VIRTUAL_TRACE();

 private:
  // PairIterable<> implementation.
  IterationSource* startIteration(ScriptState*, ExceptionState&) override;
};

}  // namespace blink
#endif
