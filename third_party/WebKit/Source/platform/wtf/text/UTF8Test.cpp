#include "platform/wtf/text/UTF8.h"

#include "base/macros.h"
#include "platform/wtf/Vector.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace WTF {
namespace Unicode {

TEST(ConvertUTF16ToUTF8, ASCII) {
  UChar source[] = {'a', 'b', 'c'};
  Vector<char> target(3);
  const UChar* source_start = source;
  char* target_start = target.data();
  EXPECT_EQ(
      kConversionOK,
      ConvertUTF16ToUTF8(&source_start, source + arraysize(source),
                         &target_start, target_start + target.size(), false));
  EXPECT_EQ(source_start, source + 3);
  EXPECT_EQ(target_start, target.data() + 3);
  EXPECT_EQ('a', target[0]);
  EXPECT_EQ('b', target[1]);
  EXPECT_EQ('c', target[2]);
}

TEST(ConvertUTF16ToUTF8, SurrogatePair) {
  UChar source[] = {0xD83D, 0xDE00};
  Vector<char> target(4);
  const UChar* source_start = source;
  char* target_start = target.data();
  EXPECT_EQ(
      kConversionOK,
      ConvertUTF16ToUTF8(&source_start, source + arraysize(source),
                         &target_start, target_start + target.size(), false));
  EXPECT_EQ(source_start, source + 2);
  EXPECT_EQ(target_start, target.data() + 4);
  EXPECT_EQ('\xF0', target[0]);
  EXPECT_EQ('\x9F', target[1]);
  EXPECT_EQ('\x98', target[2]);
  EXPECT_EQ('\x80', target[3]);
}

TEST(ConvertUTF16ToUTF8, UnpairedHighSurrogate) {
  UChar source[] = {'a', 0xD83D};
  Vector<char> target(16);
  const UChar* source_start = source;
  char* target_start = target.data();
  EXPECT_EQ(
      kSourceExhausted,
      ConvertUTF16ToUTF8(&source_start, source + arraysize(source),
                         &target_start, target_start + target.size(), false));
  EXPECT_EQ(source_start, source + 1);
  EXPECT_EQ(target_start, target.data() + 1);
  EXPECT_EQ('a', target[0]);
}

TEST(ConvertUTF16ToUTF8, UnpairedLowSurrogate) {
  UChar source[] = {'a', 0xDE00};
  Vector<char> target(16);

  const UChar* source_start = source;
  char* target_start = target.data();
  EXPECT_EQ(
      kSourceIllegal,
      ConvertUTF16ToUTF8(&source_start, source + arraysize(source),
                         &target_start, target_start + target.size(), true));
  EXPECT_EQ(source_start, source + 1);
  EXPECT_EQ(target_start, target.data() + 1);
  EXPECT_EQ('a', target[0]);

  source_start = source;
  target_start = target.data();
  EXPECT_EQ(
      kConversionOK,
      ConvertUTF16ToUTF8(&source_start, source + arraysize(source),
                         &target_start, target_start + target.size(), false));
  EXPECT_EQ(source_start, source + 2);
  EXPECT_EQ(target_start, target.data() + 4);
  EXPECT_EQ('a', target[0]);
  EXPECT_EQ('\xED', target[1]);
  EXPECT_EQ('\xB8', target[2]);
  EXPECT_EQ('\x80', target[3]);
}

}  // namespace Unicode
}  // namespace WTF
