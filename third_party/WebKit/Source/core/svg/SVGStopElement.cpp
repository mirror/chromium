/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "core/svg/SVGStopElement.h"

#include "core/layout/svg/LayoutSVGResourceContainer.h"

namespace blink {

inline SVGStopElement::SVGStopElement(Document& document)
    : SVGElement(SVGNames::stopTag, document),
      offset_(SVGAnimatedNumber::Create(this,
                                        SVGNames::offsetAttr,
                                        SVGNumberAcceptPercentage::Create())) {
  AddToPropertyMap(offset_);

  // Since stop elements don't have corresponding layout objects, we rely on
  // style recalc callbacks for invalidation.
  SetHasCustomStyleCallbacks();
}

void SVGStopElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(offset_);
  SVGElement::Trace(visitor);
}

DEFINE_NODE_FACTORY(SVGStopElement)

namespace {

void InvalidateInstancesAndAncestorResources(SVGStopElement* stop_element) {
  SVGElement::InvalidationGuard invalidation_guard(stop_element);

  for (auto* e = stop_element->parentElement(); e; e = e->parentElement()) {
    auto* layout_object = e->GetLayoutObject();
    if (!layout_object || !layout_object->IsSVGResourceContainer())
      continue;

    ToLayoutSVGResourceContainer(layout_object)->RemoveAllClientsFromCache();
    LayoutSVGResourceContainer::MarkForLayoutAndParentResourceInvalidation(
        layout_object);
  }
}

}  // namespace

void SVGStopElement::SvgAttributeChanged(const QualifiedName& attr_name) {
  if (attr_name == SVGNames::offsetAttr) {
    InvalidateInstancesAndAncestorResources(this);
    return;
  }

  SVGElement::SvgAttributeChanged(attr_name);
}

void SVGStopElement::DidRecalcStyle() {
  SVGElement::DidRecalcStyle();

  InvalidateInstancesAndAncestorResources(this);
}

Color SVGStopElement::StopColorIncludingOpacity() const {
  const ComputedStyle* style = NonLayoutObjectComputedStyle();
  // FIXME: This check for null style exists to address Bug WK 90814, a rare
  // crash condition in which the layoutObject or style is null. This entire
  // class is scheduled for removal (Bug WK 86941) and we will tolerate this
  // null check until then.
  if (!style)
    return Color(Color::kTransparent);  // Transparent black.

  const SVGComputedStyle& svg_style = style->SvgStyle();
  return svg_style.StopColor().CombineWithAlpha(svg_style.StopOpacity());
}

}  // namespace blink
