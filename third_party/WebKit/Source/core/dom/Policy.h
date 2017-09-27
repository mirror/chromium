#ifndef Policy_h
#define Policy_h

#include "core/CoreExport.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/feature_policy/FeaturePolicy.h"
#include "platform/heap/GarbageCollected.h"
#include "platform/heap/Member.h"

namespace blink {

class Document;

class CORE_EXPORT Policy final : public GarbageCollectedFinalized<Policy>,
                                 public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  Policy() {}
  ~Policy() {}
  static Policy* Create(Document*);
  bool allowsFeature(const String&) const;
  bool allowsFeature(const String&, const String&) const;
  Vector<String> allowedFeatures() const;
  Vector<String> getAllowlist(const String&) const;
  DECLARE_TRACE();

 private:
  Policy(Document*);
  Member<Document> document_;
  const FeatureNameMap& feature_names = GetDefaultFeatureNameMap();
};

}  // namespace blink

#endif
