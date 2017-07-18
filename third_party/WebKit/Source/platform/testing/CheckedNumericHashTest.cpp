#include "platform/wtf/CheckedNumeric.h"
#include "platform/wtf/HashFunctions.h"
#include "platform/wtf/HashMap.h"
#include "platform/wtf/HashTableDeletedValueType.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class ClassUsingCheckedInt16 {
 public:
  ClassUsingCheckedInt16() = default;

  explicit ClassUsingCheckedInt16(int16_t a)
      : value_(a), is_deleted_value(false) {
    CHECK(value_.IsValid());
  }

  explicit ClassUsingCheckedInt16(WTF::HashTableDeletedValueType)
      : is_deleted_value(true) {}

  bool operator==(const ClassUsingCheckedInt16& other) const {
    return value_.ValueOrDie() == value_.ValueOrDie();
  }

  int16_t GetValue() const { return value_.ValueOrDie(); }

  bool IsHashTableDeletedValue() const { return is_deleted_value; }

 private:
  WTF::CheckedNumeric<int16_t> value_;
  bool is_deleted_value;
};

struct ClassUsingCheckedInt16Hash {
  static unsigned GetHash(const ClassUsingCheckedInt16& key) {
    return WTF::HashInt(static_cast<uint16_t>(key.GetValue()));
  };

  static bool Equal(const ClassUsingCheckedInt16& a,
                    const ClassUsingCheckedInt16& b) {
    return a == b;
  }

  static const bool safe_to_compare_to_empty_or_deleted = true;
};

}  // namespace blink

namespace WTF {

template <>
struct DefaultHash<blink::ClassUsingCheckedInt16> {
  STATIC_ONLY(DefaultHash);
  typedef blink::ClassUsingCheckedInt16Hash Hash;
};

template <>
struct HashTraits<blink::ClassUsingCheckedInt16>
    : SimpleClassHashTraits<blink::ClassUsingCheckedInt16> {
  STATIC_ONLY(HashTraits);
};

}  // namespace WTF

namespace blink {

using HashMapForClassUsingCheckedInt16 =
    WTF::HashMap<ClassUsingCheckedInt16, bool>;

TEST(CheckedNumericHash, EmptyValueProblem) {
  HashMapForClassUsingCheckedInt16 hash_map;
  ClassUsingCheckedInt16 key(1);
  hash_map.insert(key, true);
}

}  // namespace blink
