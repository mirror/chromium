// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/cssom/InlineStylePropertyMap.h"

#include "bindings/core/v8/Iterable.h"
#include "core/CSSPropertyNames.h"
#include "core/css/CSSPrimitiveValue.h"
#include "core/css/CSSPropertyMetadata.h"
#include "core/css/CSSValueList.h"
#include "core/css/StylePropertySet.h"
#include "core/css/cssom/CSSOMTypes.h"
#include "core/css/cssom/CSSSimpleLength.h"

namespace blink {

StylePropertyMap::StyleValueVector InlineStylePropertyMap::getAll(CSSPropertyID propertyID)
{
    CSSValue* cssValue = m_ownerElement->ensureMutableInlineStyle().getPropertyCSSValue(propertyID);
    if (!cssValue)
        return StyleValueVector();

    return cssValueToStyleValueVector(propertyID, *cssValue);
}

Vector<String> InlineStylePropertyMap::getProperties()
{
    Vector<String> result;
    StylePropertySet& inlineStyleSet = m_ownerElement->ensureMutableInlineStyle();
    for (unsigned i = 0; i < inlineStyleSet.propertyCount(); i++) {
        CSSPropertyID propertyID = inlineStyleSet.propertyAt(i).id();
        result.append(getPropertyNameString(propertyID));
    }
    return result;
}

void InlineStylePropertyMap::set(CSSPropertyID propertyID, StyleValueOrStyleValueSequenceOrString& item, ExceptionState& exceptionState)
{
    if (item.isStyleValue()) {
        StyleValue* styleValue = item.getAsStyleValue();
        if (!CSSOMTypes::propertyCanTake(propertyID, *styleValue))  {
            exceptionState.throwTypeError("Invalid type for property");
            return;
        }
        m_ownerElement->setInlineStyleProperty(propertyID, styleValue->toCSSValue());
    } else if (item.isStyleValueSequence()) {
        if (!CSSPropertyMetadata::propertySupportsMultiple(propertyID)) {
            exceptionState.throwTypeError("Property does not support multiple values");
            return;
        }

        // TODO(meade): This won't always work. Figure out what kind of CSSValueList to create properly.
        CSSValueList* valueList = CSSValueList::createSpaceSeparated();
        StyleValueVector styleValueVector = item.getAsStyleValueSequence();
        for (const Member<StyleValue> value : styleValueVector) {
            if (!CSSOMTypes::propertyCanTake(propertyID, *value)) {
                exceptionState.throwTypeError("Invalid type for property");
                return;
            }
            valueList->append(value->toCSSValue());
        }

        m_ownerElement->setInlineStyleProperty(propertyID, valueList);
    } else {
        // Parse it.
        ASSERT(item.isString());
        // TODO(meade): Implement this.
        exceptionState.throwTypeError("Not implemented yet");
    }
}

void InlineStylePropertyMap::append(CSSPropertyID propertyID, StyleValueOrStyleValueSequenceOrString& item, ExceptionState& exceptionState)
{
    if (!CSSPropertyMetadata::propertySupportsMultiple(propertyID)) {
        exceptionState.throwTypeError("Property does not support multiple values");
        return;
    }

    CSSValue* cssValue = m_ownerElement->ensureMutableInlineStyle().getPropertyCSSValue(propertyID);
    CSSValueList* cssValueList = nullptr;
    if (cssValue->isValueList()) {
        cssValueList = toCSSValueList(cssValue);
    } else {
        // TODO(meade): Figure out what the correct behaviour here is.
        exceptionState.throwTypeError("Property is not already list valued");
        return;
    }

    if (item.isStyleValue()) {
        StyleValue* styleValue = item.getAsStyleValue();
        if (!CSSOMTypes::propertyCanTake(propertyID, *styleValue)) {
            exceptionState.throwTypeError("Invalid type for property");
            return;
        }
        cssValueList->append(item.getAsStyleValue()->toCSSValue());
    } else if (item.isStyleValueSequence()) {
        for (StyleValue* styleValue : item.getAsStyleValueSequence()) {
            if (!CSSOMTypes::propertyCanTake(propertyID, *styleValue)) {
                exceptionState.throwTypeError("Invalid type for property");
                return;
            }
            cssValueList->append(styleValue->toCSSValue());
        }
    } else {
        // Parse it.
        ASSERT(item.isString());
        // TODO(meade): Implement this.
        exceptionState.throwTypeError("Not implemented yet");
        return;
    }

    m_ownerElement->setInlineStyleProperty(propertyID, cssValueList);
}

void InlineStylePropertyMap::remove(CSSPropertyID propertyID, ExceptionState& exceptionState)
{
    m_ownerElement->removeInlineStyleProperty(propertyID);
}

HeapVector<StylePropertyMap::StylePropertyMapEntry> InlineStylePropertyMap::getIterationEntries()
{
    HeapVector<StylePropertyMap::StylePropertyMapEntry> result;
    StylePropertySet& inlineStyleSet = m_ownerElement->ensureMutableInlineStyle();
    for (unsigned i = 0; i < inlineStyleSet.propertyCount(); i++) {
        StylePropertySet::PropertyReference propertyReference = inlineStyleSet.propertyAt(i);
        CSSPropertyID propertyID = propertyReference.id();
        StyleValueVector styleValueVector = cssValueToStyleValueVector(propertyID, *propertyReference.value());
        StyleValueOrStyleValueSequence value;
        if (styleValueVector.size() == 1)
            value.setStyleValue(styleValueVector[0]);
        else
            value.setStyleValueSequence(styleValueVector);
        result.append(std::make_pair(getPropertyNameString(propertyID), value));
    }
    return result;
}

} // namespace blink

