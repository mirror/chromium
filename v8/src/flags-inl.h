// Copyright 2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_FLAGS_INL_H_
#define V8_FLAGS_INL_H_

namespace v8 { namespace internal {

bool* Flag::bool_variable() const  {
  ASSERT(type_ == BOOL);
  return &variable_->b;
}

int* Flag::int_variable() const {
  ASSERT(type_ == INT);
  return &variable_->i;
}

double* Flag::float_variable() const {
  ASSERT(type_ == FLOAT);
  return &variable_->f;
}

const char** Flag::string_variable() const {
  ASSERT(type_ == STRING);
  return &variable_->s;
}

bool Flag::bool_default() const {
  ASSERT(type_ == BOOL);
  return default_.b;
}

int Flag::int_default() const {
  ASSERT(type_ == INT);
  return default_.i;
}

double Flag::float_default() const {
  ASSERT(type_ == FLOAT);
  return default_.f;
}

const char* Flag::string_default() const {
  ASSERT(type_ == STRING);
  return default_.s;
}

} }  // namespace v8::internal

#endif  // V8_FLAGS_INL_H_
