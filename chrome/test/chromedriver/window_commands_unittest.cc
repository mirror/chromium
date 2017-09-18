// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/window_commands.h"

#include <memory>
#include <string>

#include "base/values.h"
#include "chrome/test/chromedriver/chrome/status.h"
#include "chrome/test/chromedriver/session.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(WindowCommandsTest, ProcessInputActionSequenceNone) {
  Session session("1");
  base::DictionaryValue action_sequence;
  std::unique_ptr<base::ListValue> result(new base::ListValue());

  std::unique_ptr<base::ListValue> actions(new base::ListValue());
  std::unique_ptr<base::DictionaryValue> action(new base::DictionaryValue());
  action->SetString("type", "pause");
  action->SetInteger("duration", 4);
  actions->Append(std::move(action));

  // key type action
  action_sequence.SetString("type", "none");
  action_sequence.SetString("id", "17");
  action_sequence.SetList("actions", std::move(actions));

  Status status =
      ProcessInputActionSequence(&session, &action_sequence, &result);
  ASSERT_TRUE(status.IsOk());

  // check resulting action dictionary
  base::DictionaryValue* result_action;
  std::string result_id;
  std::string result_type;
  std::string result_subtype;
  int result_duration;
  const base::DictionaryValue* state;
  const base::DictionaryValue* source;
  std::string source_id;
  std::string source_type;

  const unsigned long length = 1;
  ASSERT_EQ(length, result->GetSize());
  result->GetDictionary(0, &result_action);
  result_action->GetString("id", &result_id);
  result_action->GetString("type", &result_type);
  result_action->GetString("subtype", &result_subtype);
  result_action->GetInteger("duration", &result_duration);
  session.input_state_table->GetDictionary("17", &state);
  session.active_input_sources->GetDictionary(0, &source);
  source->GetString("id", &source_id);
  source->GetString("type", &source_type);

  ASSERT_EQ("17", result_id);
  ASSERT_EQ("none", result_type);
  ASSERT_EQ("pause", result_subtype);
  ASSERT_EQ(4, result_duration);
  ASSERT_TRUE(state->empty());
  ASSERT_EQ("17", source_id);
  ASSERT_EQ("none", source_type);

  std::unique_ptr<base::ListValue> bad_actions(new base::ListValue());
  std::unique_ptr<base::DictionaryValue> bad_action(
      new base::DictionaryValue());
  bad_action->SetString("type", "pause");
  bad_action->SetInteger("duration", -1);
  bad_actions->Append(std::move(bad_action));

  action_sequence.SetList("actions", std::move(bad_actions));
  status = ProcessInputActionSequence(&session, &action_sequence, &result);
  ASSERT_TRUE(status.IsError());
  ASSERT_EQ(status.code(), kInvalidArgument);
}

TEST(WindowCommandsTest, ProcessInputActionSequenceKey) {
  Session session("1");
  base::DictionaryValue action_sequence;
  std::unique_ptr<base::ListValue> result(new base::ListValue());

  std::unique_ptr<base::ListValue> actions(new base::ListValue());
  std::unique_ptr<base::DictionaryValue> action(new base::DictionaryValue());
  action->SetString("type", "keyUp");
  action->SetString("value", "\uE000");
  actions->Append(std::move(action));

  // key type action
  action_sequence.SetString("type", "key");
  action_sequence.SetString("id", "17");
  action_sequence.SetList("actions", std::move(actions));

  Status status =
      ProcessInputActionSequence(&session, &action_sequence, &result);
  ASSERT_TRUE(status.IsOk());

  // check resulting action dictionary
  base::DictionaryValue* result_action;
  std::string result_id;
  std::string result_type;
  std::string result_subtype;
  std::string result_value;
  const base::DictionaryValue* state;
  const base::DictionaryValue* source;
  std::string source_id;
  std::string source_type;

  const unsigned long length = 1;
  ASSERT_EQ(length, result->GetSize());
  result->GetDictionary(0, &result_action);
  result_action->GetString("id", &result_id);
  result_action->GetString("type", &result_type);
  result_action->GetString("subtype", &result_subtype);
  result_action->GetString("value", &result_value);
  session.input_state_table->GetDictionary("17", &state);
  session.active_input_sources->GetDictionary(0, &source);
  source->GetString("id", &source_id);
  source->GetString("type", &source_type);

  ASSERT_EQ("17", result_id);
  ASSERT_EQ("key", result_type);
  ASSERT_EQ("keyUp", result_subtype);
  ASSERT_EQ("\uE000", result_value);

  const base::ListValue* pressed;
  bool alt;
  bool shift;
  bool ctrl;
  bool meta;

  state->GetList("pressed", &pressed);
  state->GetBoolean("alt", &alt);
  state->GetBoolean("shift", &shift);
  state->GetBoolean("ctrl", &ctrl);
  state->GetBoolean("meta", &meta);
  ASSERT_TRUE(pressed->empty());
  ASSERT_FALSE(alt);
  ASSERT_FALSE(shift);
  ASSERT_FALSE(ctrl);
  ASSERT_FALSE(meta);
  ASSERT_EQ("17", source_id);
  ASSERT_EQ("key", source_type);

  std::unique_ptr<base::ListValue> bad_actions(new base::ListValue());
  std::unique_ptr<base::DictionaryValue> bad_action(
      new base::DictionaryValue());
  bad_action->SetString("type", "keyUpper");
  bad_action->SetString("value", "\uE000");
  bad_actions->Append(std::move(bad_action));

  action_sequence.SetList("actions", std::move(bad_actions));
  status = ProcessInputActionSequence(&session, &action_sequence, &result);
  ASSERT_TRUE(status.IsError());
  ASSERT_EQ(status.code(), kInvalidArgument);
}

TEST(WindowCommandsTest, ProcessInputActionSequencePointer) {
  Session session("1");
  base::DictionaryValue action_sequence;
  std::unique_ptr<base::ListValue> result(new base::ListValue());

  std::unique_ptr<base::ListValue> actions(new base::ListValue());
  std::unique_ptr<base::DictionaryValue> action(new base::DictionaryValue());
  std::unique_ptr<base::DictionaryValue> parameters(
      new base::DictionaryValue());
  parameters->SetString("pointerType", "mouse");
  action->SetString("type", "pointerUp");
  action->SetInteger("button", 4);
  actions->Append(std::move(action));

  // key type action
  action_sequence.SetString("type", "pointer");
  action_sequence.SetString("id", "17");
  action_sequence.SetDictionary("parameters", std::move(parameters));
  action_sequence.SetList("actions", std::move(actions));

  Status status =
      ProcessInputActionSequence(&session, &action_sequence, &result);
  ASSERT_TRUE(status.IsOk());

  // check resulting action dictionary
  base::DictionaryValue* result_action;
  std::string result_id;
  std::string result_type;
  std::string result_subtype;
  int result_button;
  const base::DictionaryValue* state;
  const base::DictionaryValue* source;
  std::string source_id;
  std::string source_type;

  const unsigned long length = 1;
  ASSERT_EQ(length, result->GetSize());
  result->GetDictionary(0, &result_action);
  result_action->GetString("id", &result_id);
  result_action->GetString("type", &result_type);
  result_action->GetString("subtype", &result_subtype);
  result_action->GetInteger("button", &result_button);
  session.input_state_table->GetDictionary("17", &state);
  session.active_input_sources->GetDictionary(0, &source);
  source->GetString("id", &source_id);
  source->GetString("type", &source_type);

  ASSERT_EQ("17", result_id);
  ASSERT_EQ("pointer", result_type);
  ASSERT_EQ("pointerUp", result_subtype);
  ASSERT_EQ(4, result_button);

  const base::ListValue* pressed;
  std::string state_subtype;
  int x;
  int y;

  state->GetList("pressed", &pressed);
  state->GetInteger("x", &x);
  state->GetInteger("y", &y);
  state->GetString("subtype", &state_subtype);
  ASSERT_TRUE(pressed->empty());
  ASSERT_EQ("mouse", state_subtype);
  ASSERT_EQ(x, 0);
  ASSERT_EQ(y, 0);
  ASSERT_EQ("17", source_id);
  ASSERT_EQ("pointer", source_type);

  std::unique_ptr<base::ListValue> bad_actions(new base::ListValue());
  std::unique_ptr<base::DictionaryValue> bad_action(
      new base::DictionaryValue());
  bad_action->SetString("type", "pointerUp");
  bad_action->SetInteger("button", -1);
  bad_actions->Append(std::move(bad_action));

  action_sequence.SetList("actions", std::move(bad_actions));
  status = ProcessInputActionSequence(&session, &action_sequence, &result);
  ASSERT_TRUE(status.IsError());
  ASSERT_EQ(status.code(), kInvalidArgument);
}
