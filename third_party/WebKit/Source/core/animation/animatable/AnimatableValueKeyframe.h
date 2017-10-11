// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AnimatableValueKeyframe_h
#define AnimatableValueKeyframe_h

#include "core/CoreExport.h"
#include "core/animation/Keyframe.h"
#include "core/animation/animatable/AnimatableValue.h"
#include "core/css/CSSPropertyIDTemplates.h"

namespace blink {

// TODO(alancutter): Delete this class once TransitionKeyframe has completely
// replaced it.
class CORE_EXPORT AnimatableValueKeyframe : public Keyframe {
 public:
  static scoped_refptr<AnimatableValueKeyframe> Create() {
    return WTF::AdoptRef(new AnimatableValueKeyframe);
  }
  void SetPropertyValue(CSSPropertyID property,
                        scoped_refptr<AnimatableValue> value) {
    property_values_.Set(property, std::move(value));
  }
  void ClearPropertyValue(CSSPropertyID property) {
    property_values_.erase(property);
  }
  AnimatableValue* PropertyValue(CSSPropertyID property) const {
    DCHECK(property_values_.Contains(property));
    return property_values_.at(property);
  }
  PropertyHandleSet Properties() const override;

  class PropertySpecificKeyframe : public Keyframe::PropertySpecificKeyframe {
   public:
    static scoped_refptr<PropertySpecificKeyframe> Create(
        double offset,
        scoped_refptr<TimingFunction> easing,
        scoped_refptr<AnimatableValue> value,
        EffectModel::CompositeOperation composite) {
      return WTF::AdoptRef(new PropertySpecificKeyframe(
          offset, std::move(easing), std::move(value), composite));
    }

    AnimatableValue* Value() const { return value_.get(); }
    const AnimatableValue* GetAnimatableValue() const final {
      return value_.get();
    }

    bool IsNeutral() const final { return false; }
    scoped_refptr<Keyframe::PropertySpecificKeyframe> NeutralKeyframe(
        double offset,
        scoped_refptr<TimingFunction> easing) const final {
      NOTREACHED();
      return nullptr;
    }
    scoped_refptr<Interpolation> CreateInterpolation(
        const PropertyHandle&,
        const Keyframe::PropertySpecificKeyframe& end) const final;

   private:
    PropertySpecificKeyframe(double offset,
                             scoped_refptr<TimingFunction> easing,
                             scoped_refptr<AnimatableValue> value,
                             EffectModel::CompositeOperation composite)
        : Keyframe::PropertySpecificKeyframe(offset,
                                             std::move(easing),
                                             composite),
          value_(std::move(value)) {}

    scoped_refptr<Keyframe::PropertySpecificKeyframe> CloneWithOffset(
        double offset) const override;
    bool IsAnimatableValuePropertySpecificKeyframe() const override {
      return true;
    }

    scoped_refptr<AnimatableValue> value_;
  };

 private:
  AnimatableValueKeyframe() {}

  AnimatableValueKeyframe(const AnimatableValueKeyframe& copy_from);

  scoped_refptr<Keyframe> Clone() const override;
  scoped_refptr<Keyframe::PropertySpecificKeyframe>
  CreatePropertySpecificKeyframe(const PropertyHandle&) const override;

  bool IsAnimatableValueKeyframe() const override { return true; }

  using PropertyValueMap =
      HashMap<CSSPropertyID, scoped_refptr<AnimatableValue>>;
  PropertyValueMap property_values_;
};

using AnimatableValuePropertySpecificKeyframe =
    AnimatableValueKeyframe::PropertySpecificKeyframe;

DEFINE_TYPE_CASTS(AnimatableValueKeyframe,
                  Keyframe,
                  value,
                  value->IsAnimatableValueKeyframe(),
                  value.IsAnimatableValueKeyframe());
DEFINE_TYPE_CASTS(AnimatableValuePropertySpecificKeyframe,
                  Keyframe::PropertySpecificKeyframe,
                  value,
                  value->IsAnimatableValuePropertySpecificKeyframe(),
                  value.IsAnimatableValuePropertySpecificKeyframe());

}  // namespace blink

#endif
