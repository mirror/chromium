/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IDBKey_h
#define IDBKey_h

#include <memory>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include "modules/ModulesExport.h"
#include "platform/SharedBuffer.h"
#include "platform/wtf/Forward.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class MODULES_EXPORT IDBKey {
 public:
  typedef Vector<std::unique_ptr<IDBKey>> KeyArray;

  static std::unique_ptr<IDBKey> CreateInvalid() {
    return WTF::WrapUnique(new IDBKey());
  }

  static std::unique_ptr<IDBKey> CreateNumber(double number) {
    return WTF::WrapUnique(new IDBKey(kNumberType, number));
  }

  static std::unique_ptr<IDBKey> CreateBinary(
      scoped_refptr<SharedBuffer> binary) {
    return WTF::WrapUnique(new IDBKey(std::move(binary)));
  }

  static std::unique_ptr<IDBKey> CreateString(const String& string) {
    return WTF::WrapUnique(new IDBKey(string));
  }

  static std::unique_ptr<IDBKey> CreateDate(double date) {
    return WTF::WrapUnique(new IDBKey(kDateType, date));
  }

  static std::unique_ptr<IDBKey> CreateArray(KeyArray array) {
    return WTF::WrapUnique(new IDBKey(std::move(array)));
  }

  ~IDBKey();

  // In order of the least to the highest precedent in terms of sort order.
  // These values are written to logs. New enum values can be added, but
  // existing enums must never be renumbered or deleted and reused.
  enum Type {
    kInvalidType = 0,
    kArrayType = 1,
    kBinaryType = 2,
    kStringType = 3,
    kDateType = 4,
    kNumberType = 5,
    kTypeEnumMax,
  };

  Type GetType() const { return type_; }
  bool IsValid() const;

  const KeyArray& Array() const {
    DCHECK_EQ(type_, kArrayType);
    return array_;
  }

  scoped_refptr<SharedBuffer> Binary() const {
    DCHECK_EQ(type_, kBinaryType);
    return binary_;
  }

  const String& GetString() const {
    DCHECK_EQ(type_, kStringType);
    return string_;
  }

  double Date() const {
    DCHECK_EQ(type_, kDateType);
    return number_;
  }

  double Number() const {
    DCHECK_EQ(type_, kNumberType);
    return number_;
  }

  int Compare(const IDBKey* other) const;
  bool IsLessThan(const IDBKey* other) const;
  bool IsEqual(const IDBKey* other) const;

  // Returns a new key array with invalid keys and duplicates removed.
  //
  // The items in the key array are moved out of the given IDBKey, which must be
  // an array. For this reason, the method is a static method that receives its
  // argument via an std::unique_ptr.
  static KeyArray ToMultiEntryArray(std::unique_ptr<IDBKey> array_key);

  // Moves the key array out of a given IDBKey.
  static KeyArray TakeArray(std::unique_ptr<IDBKey> array_key) {
    DCHECK_EQ(array_key->type_, Type::kArrayType);
    return std::move(array_key->array_);
  }

 private:
  IDBKey() : type_(kInvalidType) {}
  IDBKey(Type type, double number) : type_(type), number_(number) {}
  explicit IDBKey(const String& value) : type_(kStringType), string_(value) {}
  explicit IDBKey(scoped_refptr<SharedBuffer> value)
      : type_(kBinaryType), binary_(std::move(value)) {}
  explicit IDBKey(KeyArray key_array)
      : type_(kArrayType), array_(std::move(key_array)) {}

  Type type_;
  KeyArray array_;
  scoped_refptr<SharedBuffer> binary_;
  const String string_;
  const double number_ = 0;
};

}  // namespace blink

#endif  // IDBKey_h
