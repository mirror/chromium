// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ObjectPaintProperties_h
#define ObjectPaintProperties_h

#include "core/CoreExport.h"
#include "platform/geometry/LayoutPoint.h"
#include "platform/graphics/paint/ClipPaintPropertyNode.h"
#include "platform/graphics/paint/EffectPaintPropertyNode.h"
#include "platform/graphics/paint/PaintChunkProperties.h"
#include "platform/graphics/paint/PropertyTreeState.h"
#include "platform/graphics/paint/ScrollPaintPropertyNode.h"
#include "platform/graphics/paint/TransformPaintPropertyNode.h"
#include "wtf/PassRefPtr.h"
#include "wtf/PtrUtil.h"
#include "wtf/RefPtr.h"
#include <memory>

namespace blink {

// This class stores property tree related information associated with a
// LayoutObject.
// Currently there are two groups of information:
// 1. The set of property nodes created locally by this LayoutObject.
// 2. The set of property nodes (inherited, or created locally) and paint offset
//    that can be used to paint the border box of this LayoutObject (see:
//    localBorderBoxProperties).
class CORE_EXPORT ObjectPaintProperties {
  WTF_MAKE_NONCOPYABLE(ObjectPaintProperties);
  USING_FAST_MALLOC(ObjectPaintProperties);

 public:
  static std::unique_ptr<ObjectPaintProperties> create() {
    return WTF::wrapUnique(new ObjectPaintProperties());
  }

  // The hierarchy of the transform subtree created by a LayoutObject is as
  // follows:
  // [ paintOffsetTranslation ]           Normally paint offset is accumulated
  // |                                    without creating a node until we see,
  // |                                    for example, transform or
  // |                                    position:fixed.
  // +---[ transform ]                    The space created by CSS transform.
  //     |                                This is the local border box space,
  //     |                                see: localBorderBoxProperties below.
  //     +---[ perspective ]              The space created by CSS perspective.
  //     |   +---[ svgLocalToBorderBoxTransform ] Additional transform for
  //                                      children of the outermost root SVG.
  //     |              OR                (SVG does not support scrolling.)
  //     |   +---[ scrollTranslation ]    The space created by overflow clip.
  //     +---[ scrollbarPaintOffset ]     TODO(trchen): Remove this once we bake
  //                                      the paint offset into frameRect.  This
  //                                      is equivalent to the local border box
  //                                      space above, with pixel snapped paint
  //                                      offset baked in. It is really
  //                                      redundant, but it is a pain to teach
  //                                      scrollbars to paint with an offset.
  const TransformPaintPropertyNode* paintOffsetTranslation() const {
    return m_paintOffsetTranslation.get();
  }
  const TransformPaintPropertyNode* transform() const {
    return m_transform.get();
  }
  const TransformPaintPropertyNode* perspective() const {
    return m_perspective.get();
  }
  const TransformPaintPropertyNode* svgLocalToBorderBoxTransform() const {
    return m_svgLocalToBorderBoxTransform.get();
  }
  const TransformPaintPropertyNode* scrollTranslation() const {
    return m_scrollTranslation.get();
  }
  const TransformPaintPropertyNode* scrollbarPaintOffset() const {
    return m_scrollbarPaintOffset.get();
  }

  // Auxiliary scrolling information. Includes information such as the hierarchy
  // of scrollable areas, the extent that can be scrolled, etc. The actual
  // scroll offset is stored in the transform tree (m_scrollTranslation).
  const ScrollPaintPropertyNode* scroll() const { return m_scroll.get(); }

  const EffectPaintPropertyNode* effect() const { return m_effect.get(); }

  // The hierarchy of the clip subtree created by a LayoutObject is as follows:
  // [ css clip ]
  // [ css clip fixed position]
  // [ inner border radius clip ] Clip created by a rounded border with overflow
  //                              clip. This clip is not inset by scrollbars.
  // +--- [ overflow clip ]       Clip created by overflow clip and is inset by
  //                              the scrollbars.
  const ClipPaintPropertyNode* cssClip() const { return m_cssClip.get(); }
  const ClipPaintPropertyNode* cssClipFixedPosition() const {
    return m_cssClipFixedPosition.get();
  }
  const ClipPaintPropertyNode* innerBorderRadiusClip() const {
    return m_innerBorderRadiusClip.get();
  }
  const ClipPaintPropertyNode* overflowClip() const {
    return m_overflowClip.get();
  }

  // The complete set of property tree nodes (inherited, or created locally) and
  // paint offset that can be used to paint. |paintOffset| is relative to the
  // propertyTreeState's transform space.
  // See: localBorderBoxProperties and contentsProperties.
  struct PropertyTreeStateWithOffset {
    PropertyTreeStateWithOffset(LayoutPoint offset, PropertyTreeState treeState)
        : paintOffset(offset), propertyTreeState(treeState) {}
    LayoutPoint paintOffset;
    PropertyTreeState propertyTreeState;
  };

  // This is a complete set of property nodes and paint offset that should be
  // used as a starting point to paint this layout object. This is cached
  // because some properties inherit from the containing block chain instead of
  // the painting parent and cannot be derived in O(1) during the paint walk.
  // For example, <div style='opacity: 0.3; position: relative; margin: 11px;'/>
  // would have a paint offset of (11px, 11px) and propertyTreeState.effect()
  // would be an effect node with opacity of 0.3 which was created by the div
  // itself. Note that propertyTreeState.transform() would not be null but would
  // instead point to the transform space setup by div's ancestors.
  const PropertyTreeStateWithOffset* localBorderBoxProperties() const {
    return m_localBorderBoxProperties.get();
  }
  void updateLocalBorderBoxProperties(
      LayoutPoint& paintOffset,
      const TransformPaintPropertyNode* transform,
      const ClipPaintPropertyNode* clip,
      const EffectPaintPropertyNode* effect,
      const ScrollPaintPropertyNode* scroll) {
    if (m_localBorderBoxProperties) {
      m_localBorderBoxProperties->paintOffset = paintOffset;
      m_localBorderBoxProperties->propertyTreeState.setTransform(transform);
      m_localBorderBoxProperties->propertyTreeState.setClip(clip);
      m_localBorderBoxProperties->propertyTreeState.setEffect(effect);
      m_localBorderBoxProperties->propertyTreeState.setScroll(scroll);
    } else {
      m_localBorderBoxProperties = WTF::wrapUnique(
          new ObjectPaintProperties::PropertyTreeStateWithOffset(
              paintOffset, PropertyTreeState(transform, clip, effect, scroll)));
    }
  }
  void clearLocalBorderBoxProperties() { m_localBorderBoxProperties = nullptr; }

  // This is the complete set of property nodes and paint offset that can be
  // used to paint the contents of this object. It is similar to
  // localBorderBoxProperties but includes properties (e.g., overflow clip,
  // scroll translation) that apply to contents. This is suitable for paint
  // invalidation.
  ObjectPaintProperties::PropertyTreeStateWithOffset contentsProperties() const;

  // True if an existing property was deleted, false otherwise.
  bool clearPaintOffsetTranslation() { return clear(m_paintOffsetTranslation); }
  // True if an existing property was deleted, false otherwise.
  bool clearTransform() { return clear(m_transform); }
  // True if an existing property was deleted, false otherwise.
  bool clearEffect() { return clear(m_effect); }
  // True if an existing property was deleted, false otherwise.
  bool clearCssClip() { return clear(m_cssClip); }
  // True if an existing property was deleted, false otherwise.
  bool clearCssClipFixedPosition() { return clear(m_cssClipFixedPosition); }
  // True if an existing property was deleted, false otherwise.
  bool clearInnerBorderRadiusClip() { return clear(m_innerBorderRadiusClip); }
  // True if an existing property was deleted, false otherwise.
  bool clearOverflowClip() { return clear(m_overflowClip); }
  // True if an existing property was deleted, false otherwise.
  bool clearPerspective() { return clear(m_perspective); }
  // True if an existing property was deleted, false otherwise.
  bool clearSvgLocalToBorderBoxTransform() {
    return clear(m_svgLocalToBorderBoxTransform);
  }
  // True if an existing property was deleted, false otherwise.
  bool clearScrollTranslation() { return clear(m_scrollTranslation); }
  // True if an existing property was deleted, false otherwise.
  bool clearScrollbarPaintOffset() { return clear(m_scrollbarPaintOffset); }
  // True if an existing property was deleted, false otherwise.
  bool clearScroll() { return clear(m_scroll); }

  // True if a new property was created, false if an existing one was updated.
  template <typename... Args>
  bool updatePaintOffsetTranslation(Args&&... args) {
    return update(m_paintOffsetTranslation, std::forward<Args>(args)...);
  }
  // True if a new property was created, false if an existing one was updated.
  template <typename... Args>
  bool updateTransform(Args&&... args) {
    return update(m_transform, std::forward<Args>(args)...);
  }
  // True if a new property was created, false if an existing one was updated.
  template <typename... Args>
  bool updatePerspective(Args&&... args) {
    return update(m_perspective, std::forward<Args>(args)...);
  }
  // True if a new property was created, false if an existing one was updated.
  template <typename... Args>
  bool updateSvgLocalToBorderBoxTransform(Args&&... args) {
    DCHECK(!scrollTranslation()) << "SVG elements cannot scroll so there "
                                    "should never be both a scroll translation "
                                    "and an SVG local to border box transform.";
    return update(m_svgLocalToBorderBoxTransform, std::forward<Args>(args)...);
  }
  // True if a new property was created, false if an existing one was updated.
  template <typename... Args>
  bool updateScrollTranslation(Args&&... args) {
    DCHECK(!svgLocalToBorderBoxTransform())
        << "SVG elements cannot scroll so there should never be both a scroll "
           "translation and an SVG local to border box transform.";
    return update(m_scrollTranslation, std::forward<Args>(args)...);
  }
  // True if a new property was created, false if an existing one was updated.
  template <typename... Args>
  bool updateScrollbarPaintOffset(Args&&... args) {
    return update(m_scrollbarPaintOffset, std::forward<Args>(args)...);
  }
  // True if a new property was created, false if an existing one was updated.
  template <typename... Args>
  bool updateScroll(Args&&... args) {
    return update(m_scroll, std::forward<Args>(args)...);
  }
  // True if a new property was created, false if an existing one was updated.
  template <typename... Args>
  bool updateEffect(Args&&... args) {
    return update(m_effect, std::forward<Args>(args)...);
  }
  // True if a new property was created, false if an existing one was updated.
  template <typename... Args>
  bool updateCssClip(Args&&... args) {
    return update(m_cssClip, std::forward<Args>(args)...);
  }
  // True if a new property was created, false if an existing one was updated.
  template <typename... Args>
  bool updateCssClipFixedPosition(Args&&... args) {
    return update(m_cssClipFixedPosition, std::forward<Args>(args)...);
  }
  // True if a new property was created, false if an existing one was updated.
  template <typename... Args>
  bool updateInnerBorderRadiusClip(Args&&... args) {
    return update(m_innerBorderRadiusClip, std::forward<Args>(args)...);
  }
  // True if a new property was created, false if an existing one was updated.
  template <typename... Args>
  bool updateOverflowClip(Args&&... args) {
    return update(m_overflowClip, std::forward<Args>(args)...);
  }

#if DCHECK_IS_ON()
  // Used by FindPropertiesNeedingUpdate.h for recording the current properties.
  std::unique_ptr<ObjectPaintProperties> clone() const {
    std::unique_ptr<ObjectPaintProperties> cloned = create();
    if (m_paintOffsetTranslation)
      cloned->m_paintOffsetTranslation = m_paintOffsetTranslation->clone();
    if (m_transform)
      cloned->m_transform = m_transform->clone();
    if (m_effect)
      cloned->m_effect = m_effect->clone();
    if (m_cssClip)
      cloned->m_cssClip = m_cssClip->clone();
    if (m_cssClipFixedPosition)
      cloned->m_cssClipFixedPosition = m_cssClipFixedPosition->clone();
    if (m_innerBorderRadiusClip)
      cloned->m_innerBorderRadiusClip = m_innerBorderRadiusClip->clone();
    if (m_overflowClip)
      cloned->m_overflowClip = m_overflowClip->clone();
    if (m_perspective)
      cloned->m_perspective = m_perspective->clone();
    if (m_svgLocalToBorderBoxTransform) {
      cloned->m_svgLocalToBorderBoxTransform =
          m_svgLocalToBorderBoxTransform->clone();
    }
    if (m_scrollTranslation)
      cloned->m_scrollTranslation = m_scrollTranslation->clone();
    if (m_scrollbarPaintOffset)
      cloned->m_scrollbarPaintOffset = m_scrollbarPaintOffset->clone();
    if (m_scroll)
      cloned->m_scroll = m_scroll->clone();
    if (m_localBorderBoxProperties) {
      auto& state = m_localBorderBoxProperties->propertyTreeState;
      cloned->m_localBorderBoxProperties =
          WTF::wrapUnique(new PropertyTreeStateWithOffset(
              m_localBorderBoxProperties->paintOffset,
              PropertyTreeState(state.transform(), state.clip(), state.effect(),
                                state.scroll())));
    }
    return cloned;
  }
#endif

 private:
  ObjectPaintProperties() {}

  // True if an existing property was deleted, false otherwise.
  template <typename PaintPropertyNode>
  bool clear(RefPtr<PaintPropertyNode>& field) {
    if (field) {
      field = nullptr;
      return true;
    }
    return false;
  }

  // True if a new property was created, false if an existing one was updated.
  template <typename PaintPropertyNode, typename... Args>
  bool update(RefPtr<PaintPropertyNode>& field, Args&&... args) {
    if (field) {
      field->update(std::forward<Args>(args)...);
      return false;
    }
    field = PaintPropertyNode::create(std::forward<Args>(args)...);
    return true;
  }

  RefPtr<TransformPaintPropertyNode> m_paintOffsetTranslation;
  RefPtr<TransformPaintPropertyNode> m_transform;
  RefPtr<EffectPaintPropertyNode> m_effect;
  RefPtr<ClipPaintPropertyNode> m_cssClip;
  RefPtr<ClipPaintPropertyNode> m_cssClipFixedPosition;
  RefPtr<ClipPaintPropertyNode> m_innerBorderRadiusClip;
  RefPtr<ClipPaintPropertyNode> m_overflowClip;
  RefPtr<TransformPaintPropertyNode> m_perspective;
  // TODO(pdr): Only LayoutSVGRoot needs this and it should be moved there.
  RefPtr<TransformPaintPropertyNode> m_svgLocalToBorderBoxTransform;
  RefPtr<TransformPaintPropertyNode> m_scrollTranslation;
  RefPtr<TransformPaintPropertyNode> m_scrollbarPaintOffset;
  RefPtr<ScrollPaintPropertyNode> m_scroll;

  std::unique_ptr<PropertyTreeStateWithOffset> m_localBorderBoxProperties;
};

}  // namespace blink

#endif  // ObjectPaintProperties_h
