// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/EffectInput.h"

#include <memory>

#include "bindings/core/v8/Dictionary.h"
#include "bindings/core/v8/V8BindingForTesting.h"
#include "bindings/core/v8/V8ObjectBuilder.h"
#include "bindings/core/v8/dictionary_sequence_or_dictionary.h"
#include "core/animation/AnimationTestHelper.h"
#include "core/animation/KeyframeEffectModel.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/ExceptionCode.h"
#include "core/testing/DummyPageHolder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "v8/include/v8.h"

namespace blink {

Element* AppendElement(Document& document) {
  Element* element = document.createElement("foo");
  document.documentElement()->AppendChild(element);
  return element;
}

TEST(AnimationEffectInputTest, SortedOffsets) {
  V8TestingScope scope;
  ScriptState* script_state = scope.GetScriptState();

  V8ObjectBuilder keyframe1(script_state);
  keyframe1.AddString("width", "100px");
  keyframe1.AddString("offset", "0");

  V8ObjectBuilder keyframe2(script_state);
  keyframe2.AddString("width", "0px");
  keyframe2.AddString("offset", "1");

  Vector<ScriptValue> blink_keyframes;
  blink_keyframes.push_back(keyframe1.GetScriptValue());
  blink_keyframes.push_back(keyframe2.GetScriptValue());

  ScriptValue js_keyframes(
      script_state,
      ToV8(blink_keyframes, scope.GetContext()->Global(), scope.GetIsolate()));

  Element* element = AppendElement(scope.GetDocument());
  KeyframeEffectModelBase* effect = EffectInput::Convert(
      element, js_keyframes, EffectModel::kCompositeReplace,
      scope.GetScriptState(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  EXPECT_EQ(1.0, effect->GetFrames()[1]->CheckedOffset());
}

TEST(AnimationEffectInputTest, UnsortedOffsets) {
  V8TestingScope scope;
  ScriptState* script_state = scope.GetScriptState();

  V8ObjectBuilder keyframe1(script_state);
  keyframe1.AddString("width", "0px");
  keyframe1.AddString("offset", "1");

  V8ObjectBuilder keyframe2(script_state);
  keyframe2.AddString("width", "100px");
  keyframe2.AddString("offset", "0");

  Vector<ScriptValue> blink_keyframes;
  blink_keyframes.push_back(keyframe1.GetScriptValue());
  blink_keyframes.push_back(keyframe2.GetScriptValue());

  ScriptValue js_keyframes(
      script_state,
      ToV8(blink_keyframes, scope.GetContext()->Global(), scope.GetIsolate()));

  Element* element = AppendElement(scope.GetDocument());
  EffectInput::Convert(element, js_keyframes, EffectModel::kCompositeReplace,
                       scope.GetScriptState(), scope.GetExceptionState());
  EXPECT_TRUE(scope.GetExceptionState().HadException());
  EXPECT_EQ(kV8TypeError, scope.GetExceptionState().Code());
}

TEST(AnimationEffectInputTest, LooslySorted) {
  V8TestingScope scope;
  ScriptState* script_state = scope.GetScriptState();

  V8ObjectBuilder keyframe1(script_state);
  keyframe1.AddString("width", "100px");
  keyframe1.AddString("offset", "0");

  V8ObjectBuilder keyframe2(script_state);
  keyframe2.AddString("width", "200px");

  V8ObjectBuilder keyframe3(script_state);
  keyframe3.AddString("width", "0px");
  keyframe3.AddString("offset", "1");

  Vector<ScriptValue> blink_keyframes;
  blink_keyframes.push_back(keyframe1.GetScriptValue());
  blink_keyframes.push_back(keyframe2.GetScriptValue());
  blink_keyframes.push_back(keyframe3.GetScriptValue());

  ScriptValue js_keyframes(
      script_state,
      ToV8(blink_keyframes, scope.GetContext()->Global(), scope.GetIsolate()));

  Element* element = AppendElement(scope.GetDocument());
  KeyframeEffectModelBase* effect = EffectInput::Convert(
      element, js_keyframes, EffectModel::kCompositeReplace,
      scope.GetScriptState(), scope.GetExceptionState());
  EXPECT_FALSE(scope.GetExceptionState().HadException());
  EXPECT_EQ(1, effect->GetFrames()[2]->CheckedOffset());
}

TEST(AnimationEffectInputTest, OutOfOrderWithNullOffsets) {
  V8TestingScope scope;
  ScriptState* script_state = scope.GetScriptState();

  V8ObjectBuilder keyframe1(script_state);
  keyframe1.AddString("height", "100px");
  keyframe1.AddString("offset", "0.5");

  V8ObjectBuilder keyframe2(script_state);
  keyframe2.AddString("height", "150px");

  V8ObjectBuilder keyframe3(script_state);
  keyframe3.AddString("height", "200px");
  keyframe3.AddString("offset", "0");

  V8ObjectBuilder keyframe4(script_state);
  keyframe4.AddString("height", "300px");
  keyframe4.AddString("offset", "1");

  Vector<ScriptValue> blink_keyframes;
  blink_keyframes.push_back(keyframe1.GetScriptValue());
  blink_keyframes.push_back(keyframe2.GetScriptValue());
  blink_keyframes.push_back(keyframe3.GetScriptValue());
  blink_keyframes.push_back(keyframe4.GetScriptValue());

  ScriptValue js_keyframes(
      script_state,
      ToV8(blink_keyframes, scope.GetContext()->Global(), scope.GetIsolate()));

  Element* element = AppendElement(scope.GetDocument());
  EffectInput::Convert(element, js_keyframes, EffectModel::kCompositeReplace,
                       scope.GetScriptState(), scope.GetExceptionState());
  EXPECT_TRUE(scope.GetExceptionState().HadException());
}

TEST(AnimationEffectInputTest, Invalid) {
  V8TestingScope scope;
  ScriptState* script_state = scope.GetScriptState();

  // Not loosely sorted by offset, and there exists a keyframe with null offset.
  V8ObjectBuilder keyframe1(script_state);
  keyframe1.AddString("width", "0px");
  keyframe1.AddString("offset", "1");

  V8ObjectBuilder keyframe2(script_state);
  keyframe2.AddString("width", "200px");

  V8ObjectBuilder keyframe3(script_state);
  keyframe3.AddString("width", "200px");
  keyframe3.AddString("offset", "0");

  Vector<ScriptValue> blink_keyframes;
  blink_keyframes.push_back(keyframe1.GetScriptValue());
  blink_keyframes.push_back(keyframe2.GetScriptValue());
  blink_keyframes.push_back(keyframe3.GetScriptValue());

  ScriptValue js_keyframes(
      script_state,
      ToV8(blink_keyframes, scope.GetContext()->Global(), scope.GetIsolate()));

  Element* element = AppendElement(scope.GetDocument());
  EffectInput::Convert(element, js_keyframes, EffectModel::kCompositeReplace,
                       scope.GetScriptState(), scope.GetExceptionState());
  EXPECT_TRUE(scope.GetExceptionState().HadException());
  EXPECT_EQ(kV8TypeError, scope.GetExceptionState().Code());
}

}  // namespace blink
