/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/animation/EffectInput.h"

#include "bindings/core/v8/ArrayValue.h"
#include "bindings/core/v8/Dictionary.h"
#include "bindings/core/v8/DictionaryHelperForBindings.h"
#include "bindings/core/v8/dictionary_sequence_or_dictionary.h"
#include "core/animation/AnimationInputHelpers.h"
#include "core/animation/CompositorAnimations.h"
#include "core/animation/KeyframeEffectModel.h"
#include "core/animation/StringKeyframe.h"
#include "core/css/CSSStyleSheet.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/ExceptionCode.h"
#include "core/frame/FrameConsole.h"
#include "core/frame/LocalFrame.h"
#include "core/inspector/ConsoleMessage.h"
#include "platform/wtf/ASCIICType.h"
#include "platform/wtf/HashSet.h"
#include "platform/wtf/NonCopyingSort.h"

namespace blink {

namespace {

bool GetDoubleOrNull(v8::Local<v8::Value> v8_value,
                     v8::Local<v8::Context> context,
                     double* offset) {
  DCHECK(offset);
  if (v8_value->IsNumber())
    return v8_value->NumberValue(context).To(offset);

  if (v8_value->IsNull()) {
    *offset = std::numeric_limits<double>::quiet_NaN();
    return true;
  }
  return false;
}

bool GetDoubleOrNullArray(v8::Local<v8::Value> v8_value,
                          v8::Local<v8::Context> context,
                          v8::Isolate* isolate,
                          Vector<double>& offsets) {
  if (!v8_value->IsArray())
    return false;

  v8::Local<v8::Array> v8_array = v8::Local<v8::Array>::Cast(v8_value);
  for (size_t i = 0; i < v8_array->Length(); ++i) {
    v8::Local<v8::Value> indexed_value;
    if (!v8_array->Get(context, v8::Uint32::New(isolate, i))
             .ToLocal(&indexed_value)) {
      return false;
    }
    double offset;
    if (!GetDoubleOrNull(indexed_value, context, &offset))
      return false;
    offsets.push_back(offset);
  }
  return true;
}

// TODO(smcgruer): Description.
//
// The exception_state should be checked after this function returns.
Vector<double> ExtractOffsetProperty(const Dictionary& dictionary,
                                     ExceptionState& exception_state) {
  Vector<double> offsets;
  v8::Local<v8::Value> v8_value;
  if (dictionary.Get("offset", v8_value)) {
    double offset;
    v8::Local<v8::Context> context = dictionary.V8Context();
    v8::Isolate* isolate = dictionary.GetIsolate();
    if (GetDoubleOrNull(v8_value, context, &offset)) {
      offsets.push_back(offset);
    } else if (!GetDoubleOrNullArray(v8_value, context, isolate, offsets)) {
      exception_state.ThrowTypeError(
          "offset must be a nullable double or an array of nullable doubles");
    }
  }

  return offsets;
}

Vector<String> ExtractEasingProperty(const Dictionary& dictionary,
                                     ExceptionState& exception_state) {
  Vector<String> easings;
  if (dictionary.HasProperty("easing", exception_state)) {
    String easing;
    if (DictionaryHelper::Get(dictionary, "easing", easing)) {
      easings.push_back(easing);
    } else if (!DictionaryHelper::Get(dictionary, "easing", easings)) {
      exception_state.ThrowTypeError(
          "easing must be a DOMString or an array of DOMString");
    }
  }
  return easings;
}

Vector<EffectModel::CompositeOperation> ExtractCompositeProperty(
    const Dictionary& dictionary,
    ExceptionState& exception_state) {
  Vector<EffectModel::CompositeOperation> composite_operations;
  if (dictionary.HasProperty("composite", exception_state)) {
    // Build the exception message up front, since we can throw either during
    // property retrieval or during conversion to CompositeOperation.
    String exception_message =
        "composite must be a CompositeOperation or an "
        "array of CompositeOperation";

    String composite_string;
    Vector<String> composite_strings;

    if (DictionaryHelper::Get(dictionary, "composite", composite_string)) {
      composite_strings.push_back(composite_string);
    } else if (!DictionaryHelper::Get(dictionary, "composite",
                                      composite_strings)) {
      exception_state.ThrowTypeError(exception_message);
    }

    for (String composite : composite_strings) {
      if (composite == "add") {
        composite_operations.push_back(EffectModel::kCompositeAdd);
      } else {
        // TODO(smcgruer): Support other composite modes.
        exception_state.ThrowTypeError(exception_message);
        break;
      }
    }
  }

  return composite_operations;
}

// Validates the value of |offset| and throws an exception if out of range.
bool CheckOffset(double offset,
                 double last_offset,
                 ExceptionState& exception_state) {
  // Keyframes with offsets outside the range [0.0, 1.0] are an error.
  if (std::isnan(offset)) {
    exception_state.ThrowTypeError("Non numeric offset provided");
    return false;
  }

  if (offset < 0 || offset > 1) {
    exception_state.ThrowTypeError("Offsets provided outside the range [0, 1]");
    return false;
  }

  if (offset < last_offset) {
    exception_state.ThrowTypeError(
        "Keyframes with specified offsets are not sorted");
    return false;
  }

  return true;
}

void SetKeyframeValue(Element& element,
                      StringKeyframe& keyframe,
                      const String& property,
                      const String& value,
                      ExecutionContext* execution_context) {
  StyleSheetContents* style_sheet_contents =
      element.GetDocument().ElementSheet().Contents();
  CSSPropertyID css_property =
      AnimationInputHelpers::KeyframeAttributeToCSSProperty(
          property, element.GetDocument());
  if (css_property != CSSPropertyInvalid) {
    MutableCSSPropertyValueSet::SetResult set_result =
        css_property == CSSPropertyVariable
            ? keyframe.SetCSSPropertyValue(
                  AtomicString(property),
                  element.GetDocument().GetPropertyRegistry(), value,
                  style_sheet_contents)
            : keyframe.SetCSSPropertyValue(css_property, value,
                                           style_sheet_contents);
    if (!set_result.did_parse && execution_context) {
      Document& document = ToDocument(*execution_context);
      if (document.GetFrame()) {
        document.GetFrame()->Console().AddMessage(ConsoleMessage::Create(
            kJSMessageSource, kWarningMessageLevel,
            "Invalid keyframe value for property " + property + ": " + value));
      }
    }
    return;
  }
  css_property =
      AnimationInputHelpers::KeyframeAttributeToPresentationAttribute(property,
                                                                      element);
  if (css_property != CSSPropertyInvalid) {
    keyframe.SetPresentationAttributeValue(css_property, value,
                                           style_sheet_contents);
    return;
  }
  const QualifiedName* svg_attribute =
      AnimationInputHelpers::KeyframeAttributeToSVGAttribute(property, element);
  if (svg_attribute)
    keyframe.SetSVGAttributeValue(*svg_attribute, value);
}

EffectModel* CreateEffectModelFromKeyframes(
    Element& element,
    const StringKeyframeVector& keyframes,
    ExceptionState& exception_state) {
  StringKeyframeEffectModel* keyframe_effect_model =
      StringKeyframeEffectModel::Create(keyframes,
                                        LinearTimingFunction::Shared());
  if (!RuntimeEnabledFeatures::CSSAdditiveAnimationsEnabled()) {
    for (const auto& keyframe_group :
         keyframe_effect_model->GetPropertySpecificKeyframeGroups()) {
      PropertyHandle property = keyframe_group.key;
      if (!property.IsCSSProperty())
        continue;

      for (const auto& keyframe : keyframe_group.value->Keyframes()) {
        if (keyframe->IsNeutral()) {
          exception_state.ThrowDOMException(
              kNotSupportedError, "Partial keyframes are not supported.");
          return nullptr;
        }
        if (keyframe->Composite() != EffectModel::kCompositeReplace) {
          exception_state.ThrowDOMException(
              kNotSupportedError, "Additive animations are not supported.");
          return nullptr;
        }
      }
    }
  }

  DCHECK(!exception_state.HadException());
  return keyframe_effect_model;
}

bool ExhaustDictionaryIterator(DictionaryIterator& iterator,
                               ExecutionContext* execution_context,
                               ExceptionState& exception_state,
                               Vector<Dictionary>& result) {
  while (iterator.Next(execution_context, exception_state)) {
    Dictionary dictionary;
    if (!iterator.ValueAsDictionary(dictionary, exception_state)) {
      exception_state.ThrowTypeError("Keyframes must be objects.");
      return false;
    }
    result.push_back(dictionary);
  }
  return !exception_state.HadException();
}

}  // namespace

// Spec: http://w3c.github.io/web-animations/#processing-a-keyframes-argument
EffectModel* EffectInput::Convert(
    Element* element,
    const DictionarySequenceOrDictionary& effect_input,
    ExecutionContext* execution_context,
    ExceptionState& exception_state) {
  // Returning a null EffectModel is equivalent to having no keyframes.
  // TODO(crbug.com/772014): The element is allowed to be null; remove check.
  if (effect_input.IsNull() || !element)
    return nullptr;

  // DictionarySequence is explicitly an array of dictionaries, i.e. the
  // keyframes are in array-form.
  if (effect_input.IsDictionarySequence()) {
    return ConvertArrayForm(*element, effect_input.GetAsDictionarySequence(),
                            execution_context, exception_state);
  }

  // Check whether @@iterator is defined; if so the keyframes are in array-form,
  // otherwise they are in object-form.
  //
  // TODO(smcgruer): Is this path ever taken? An object with Symbol.iterator
  // defined on it just becomes a DictionarySequence?
  const Dictionary& dictionary = effect_input.GetAsDictionary();
  DictionaryIterator iterator = dictionary.GetIterator(execution_context);
  if (!iterator.IsNull()) {
    // TODO(alancutter): Convert keyframes during iteration rather than after to
    // match spec.
    Vector<Dictionary> keyframe_dictionaries;
    if (ExhaustDictionaryIterator(iterator, execution_context, exception_state,
                                  keyframe_dictionaries)) {
      return ConvertArrayForm(*element, keyframe_dictionaries,
                              execution_context, exception_state);
    }
    return nullptr;
  }

  return ConvertObjectForm(*element, dictionary, execution_context,
                           exception_state);
}

EffectModel* EffectInput::ConvertArrayForm(
    Element& element,
    const Vector<Dictionary>& keyframe_dictionaries,
    ExecutionContext* execution_context,
    ExceptionState& exception_state) {
  StringKeyframeVector keyframes;
  double last_offset = 0;

  for (const Dictionary& keyframe_dictionary : keyframe_dictionaries) {
    scoped_refptr<StringKeyframe> keyframe = StringKeyframe::Create();

    Nullable<double> offset;
    if (DictionaryHelper::Get(keyframe_dictionary, "offset", offset) &&
        !offset.IsNull()) {
      if (!CheckOffset(offset.Get(), last_offset, exception_state))
        return nullptr;

      last_offset = offset.Get();
      keyframe->SetOffset(offset.Get());
    }

    String composite_string;
    DictionaryHelper::Get(keyframe_dictionary, "composite", composite_string);
    if (composite_string == "add")
      keyframe->SetComposite(EffectModel::kCompositeAdd);
    // TODO(alancutter): Support "accumulate" keyframe composition.

    String timing_function_string;
    if (DictionaryHelper::Get(keyframe_dictionary, "easing",
                              timing_function_string)) {
      scoped_refptr<TimingFunction> timing_function =
          AnimationInputHelpers::ParseTimingFunction(
              timing_function_string, &element.GetDocument(), exception_state);
      if (!timing_function)
        return nullptr;
      keyframe->SetEasing(timing_function);
    }

    const Vector<String>& keyframe_properties =
        keyframe_dictionary.GetPropertyNames(exception_state);
    if (exception_state.HadException())
      return nullptr;
    for (const auto& property : keyframe_properties) {
      if (property == "offset" || property == "composite" ||
          property == "easing") {
        continue;
      }

      Vector<String> values;
      if (DictionaryHelper::Get(keyframe_dictionary, property, values)) {
        exception_state.ThrowTypeError(
            "Lists of values not permitted in array-form list of keyframes");
        return nullptr;
      }

      String value;
      DictionaryHelper::Get(keyframe_dictionary, property, value);

      SetKeyframeValue(element, *keyframe.get(), property, value,
                       execution_context);
    }
    keyframes.push_back(keyframe);
  }

  DCHECK(!exception_state.HadException());

  return CreateEffectModelFromKeyframes(element, keyframes, exception_state);
}

static bool GetPropertyIndexedKeyframeValues(
    const Dictionary& keyframe_dictionary,
    const String& property,
    ExecutionContext* execution_context,
    ExceptionState& exception_state,
    Vector<String>& result) {
  DCHECK(result.IsEmpty());

  // Array of strings.
  if (DictionaryHelper::Get(keyframe_dictionary, property, result))
    return true;

  Dictionary values_dictionary;
  if (!keyframe_dictionary.Get(property, values_dictionary) ||
      values_dictionary.IsUndefinedOrNull()) {
    // Non-object.
    String value;
    DictionaryHelper::Get(keyframe_dictionary, property, value);
    result.push_back(value);
    return true;
  }

  DictionaryIterator iterator =
      values_dictionary.GetIterator(execution_context);
  if (iterator.IsNull()) {
    // Non-iterable object.
    String value;
    DictionaryHelper::Get(keyframe_dictionary, property, value);
    result.push_back(value);
    return true;
  }

  // Iterable object.
  while (iterator.Next(execution_context, exception_state)) {
    String value;
    if (!iterator.ValueAsString(value)) {
      exception_state.ThrowTypeError(
          "Unable to read keyframe value as string.");
      return false;
    }
    result.push_back(value);
  }
  return !exception_state.HadException();
}

// Implements the procedure to "process a keyframes argument" from the
// web-animations spec, for an object form keyframes argument.
//
// See http://w3c.github.io/web-animations/#process-a-keyframes-argument
EffectModel* EffectInput::ConvertObjectForm(Element& element,
                                            const Dictionary& dictionary,
                                            ExecutionContext* execution_context,
                                            ExceptionState& exception_state) {
  Vector<double> offsets = ExtractOffsetProperty(dictionary, exception_state);
  if (exception_state.HadException())
    return nullptr;

  // The web-animations spec explicitly states that easings should be kept as
  // DOMStrings here and not parsed into timing functions until later.
  Vector<String> easings = ExtractEasingProperty(dictionary, exception_state);
  if (exception_state.HadException())
    return nullptr;

  Vector<EffectModel::CompositeOperation> composite_operations =
      ExtractCompositeProperty(dictionary, exception_state);
  if (exception_state.HadException())
    return nullptr;

  // Next extract all properties from the input argument and iterate through
  // them, processing each as a list of values for that property.
  const Vector<String>& keyframe_properties =
      dictionary.GetPropertyNames(exception_state);
  if (exception_state.HadException())
    return nullptr;

  // Steps 5.2 - 5.4 state that the user agent is to:
  //
  //   * Create sets of property-specific keyframes with no offset.
  //   * Calculate computed offsets for each set of keyframes individually.
  //   * Join the sets together and merge those with identical computed offsets.
  //
  // This is equivalent to just keeping a hashmap from computed offset to a
  // single keyframe, and doing so simplifies the parsing logic.
  //
  // TODO(smcgruer): Is a double key safe; do we need a custom key comparator?
  HashMap<double, scoped_refptr<StringKeyframe>> keyframes;

  for (const auto& property : keyframe_properties) {
    if (property == "offset" || property == "composite" ||
        property == "easing") {
      continue;
    }

    Vector<String> values;
    if (!GetPropertyIndexedKeyframeValues(
            dictionary, property, execution_context, exception_state, values))
      return nullptr;

    // Now either create a keyframe (or retrieve and augment an existing one)
    // for each value this property maps to. As explained above, this loop
    // performs both the initial creation and merging mentioned in the spec.
    size_t num_keyframes = values.size();
    for (size_t i = 0; i < num_keyframes; ++i) {
      // As all offsets are null for these keyframes, the computed offset is
      // just the fractional position of each keyframe in the array.
      //
      // The only special case is that when there is only one keyframe the sole
      // computed offset is defined as 1.
      double computed_offset =
          (num_keyframes == 1) ? 1 : i / double(num_keyframes - 1);

      auto result = keyframes.insert(computed_offset, nullptr);
      if (result.is_new_entry)
        result.stored_value->value = StringKeyframe::Create();

      SetKeyframeValue(element, *result.stored_value->value.get(), property,
                       values[i], execution_context);
    }
  }

  // 5.3 Sort processed keyframes by the computed keyframe offset of each
  // keyframe in increasing order.
  //
  // TODO(smcgruer): There must be a way to sort a HashMap or at least the
  // Keys() of a hashmap without using a Vector?
  Vector<double> keys;
  for (const auto& key : keyframes.Keys())
    keys.push_back(key);
  std::sort(keys.begin(), keys.end());

  // 5.5 - 5.12 deals with assigning the user-specified offset, easing, and
  // composite properties to the keyframes.
  StringKeyframeVector results;
  for (size_t i = 0; i < keys.size(); i++) {
    auto keyframe = keyframes.at(keys[i]);

    if (i < offsets.size()) {
      keyframe->SetOffset(offsets[i]);
    }

    // At this point we have read all the properties we will read from the input
    // object, so it is safe to parse the easing strings.
    if (!easings.IsEmpty()) {
      // 5.9 If easings has fewer items than property keyframes, repeat the
      // elements in easings successively starting from the beginning of the
      // list until easings has as many items as property keyframes.
      const String& easing = easings[i % easings.size()];
      scoped_refptr<TimingFunction> timing_function =
          AnimationInputHelpers::ParseTimingFunction(
              easing, &element.GetDocument(), exception_state);
      if (!timing_function)
        return nullptr;
      keyframe->SetEasing(timing_function);
    }

    if (!composite_operations.IsEmpty()) {
      // 5.12.2 As with easings, if composite modes has fewer items than
      // property keyframes, repeat the elements in composite modes successively
      // starting from the beginning of the list until composite modes has as
      // many items as property keyframes.
      keyframe->SetComposite(
          composite_operations[i % composite_operations.size()]);
    }

    results.push_back(keyframe);
  }

  // 6. If processed keyframes is not loosely sorted by offset, throw a
  // TypeError and abort these steps.
  double previous_offset = 0.0;
  for (const auto& keyframe : results) {
    if (!IsNull(keyframe->Offset())) {
      if (keyframe->Offset() < previous_offset) {
        exception_state.ThrowTypeError(
            "Offsets must be montonically non-decreasing.");
        return nullptr;
      }
      previous_offset = keyframe->Offset();
    }
  }

  // 7. If there exist any keyframe in processed keyframes whose keyframe offset
  // is non-null and less than zero or greater than one, throw a TypeError and
  // abort these steps.
  //
  // TODO(smcgruer): The spec strictly orders steps 6 and 7; do we need to or
  // can we check this above?
  for (const auto& keyframe : results) {
    double offset = keyframe->Offset();
    if (!IsNull(offset) && (offset < 0 || offset > 1)) {
      exception_state.ThrowTypeError(
          "Offsets must be null or in the range [0,1].");
      return nullptr;
    }
  }

  // Step 8 of the spec is done above (or will be): parsing property values
  // according to syntax for the property (discarding with console warning on
  // fail) and parsing each easing property.
  //
  // TODO(smcgruer): Currently we can throw from step 8 before we would throw
  // from step 7. That is a spec violation :(.

  // 9. Parse each of the values in unused easings using the CSS syntax defined
  // for easing property of the AnimationEffectTimingReadOnly interface, and if
  // any of the values fail to parse, throw a TypeError and abort this
  // procedure.
  // TODO(smcgruer): This will be very prone to off by one, write a test
  for (size_t i = results.size(); i < easings.size(); i++) {
    scoped_refptr<TimingFunction> timing_function =
        AnimationInputHelpers::ParseTimingFunction(
            easings[i], &element.GetDocument(), exception_state);
    if (!timing_function)
      return nullptr;
  }

  DCHECK(!exception_state.HadException());

  return CreateEffectModelFromKeyframes(element, results, exception_state);
}

}  // namespace blink
