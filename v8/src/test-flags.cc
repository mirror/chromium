// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#include <stdlib.h>

#include "v8.h"

using namespace v8::internal;

DEFINE_bool(bool_flag, true, "bool_flag");
DEFINE_int(int_flag, 13, "int_flag");
DEFINE_float(float_flag, 2.5, "float-flag");
DEFINE_string(string_flag, "Hello, world!", "string-flag");


// This test must be executed first!
static void TestDefault() {
  CHECK(FLAG_bool_flag);
  CHECK_EQ(13, FLAG_int_flag);
  CHECK_EQ(2.5, FLAG_float_flag);
  CHECK_EQ(0, strcmp(FLAG_string_flag, "Hello, world!"));
}


static void SetFlagsToDefault() {
  for (Flag* f = FlagList::list(); f != NULL; f = f->next()) {
    f->SetToDefault();
  }
  TestDefault();
}


static void Test1() {
  FlagList::Print(__FILE__, false);
  FlagList::Print(NULL, true);
}


static void Test2() {
  SetFlagsToDefault();
  int argc = 7;
  char* argv[] = { "Test2", "-nobool-flag", "notaflag", "--int_flag=77",
                   "-float_flag=.25", "--string_flag", "no way!" };
  CHECK_EQ(0, FlagList::SetFlagsFromCommandLine(&argc, argv, false));
  CHECK_EQ(7, argc);
  CHECK(!FLAG_bool_flag);
  CHECK_EQ(77, FLAG_int_flag);
  CHECK_EQ(.25, FLAG_float_flag);
  CHECK_EQ(0, strcmp(FLAG_string_flag, "no way!"));
}


static void Test2b() {
  SetFlagsToDefault();
  const char* str =
      " -nobool-flag notaflag   --int_flag=77 -float_flag=.25  "
      "--string_flag   no_way!  ";
  CHECK_EQ(0, FlagList::SetFlagsFromString(str, strlen(str)));
  CHECK(!FLAG_bool_flag);
  CHECK_EQ(77, FLAG_int_flag);
  CHECK_EQ(.25, FLAG_float_flag);
  CHECK_EQ(0, strcmp(FLAG_string_flag, "no_way!"));
}


static void Test3() {
  SetFlagsToDefault();
  int argc = 8;
  char* argv[] = { "Test3", "--bool_flag", "notaflag", "--int_flag", "-666",
                   "--float_flag", "-12E10", "-string-flag=foo-bar" };
  CHECK_EQ(0, FlagList::SetFlagsFromCommandLine(&argc, argv, true));
  CHECK_EQ(2, argc);
  CHECK(FLAG_bool_flag);
  CHECK_EQ(-666, FLAG_int_flag);
  CHECK_EQ(-12E10, FLAG_float_flag);
  CHECK_EQ(0, strcmp(FLAG_string_flag, "foo-bar"));
}


static void Test3b() {
  SetFlagsToDefault();
  const char* str =
      "--bool_flag notaflag --int_flag -666 --float_flag -12E10 "
      "-string-flag=foo-bar";
  CHECK_EQ(0, FlagList::SetFlagsFromString(str, strlen(str)));
  CHECK(FLAG_bool_flag);
  CHECK_EQ(-666, FLAG_int_flag);
  CHECK_EQ(-12E10, FLAG_float_flag);
  CHECK_EQ(0, strcmp(FLAG_string_flag, "foo-bar"));
}


static void Test4() {
  SetFlagsToDefault();
  int argc = 3;
  char* argv[] = { "Test4", "--bool_flag", "--foo" };
  CHECK_EQ(2, FlagList::SetFlagsFromCommandLine(&argc, argv, true));
  CHECK_EQ(3, argc);
}


static void Test4b() {
  SetFlagsToDefault();
  const char* str = "--bool_flag --foo";
  CHECK_EQ(2, FlagList::SetFlagsFromString(str, strlen(str)));
}


static void Test5() {
  SetFlagsToDefault();
  int argc = 2;
  char* argv[] = { "Test5", "--int_flag=\"foobar\"" };
  CHECK_EQ(1, FlagList::SetFlagsFromCommandLine(&argc, argv, true));
  CHECK_EQ(2, argc);
}


static void Test5b() {
  SetFlagsToDefault();
  const char* str = "                     --int_flag=\"foobar\"";
  CHECK_EQ(1, FlagList::SetFlagsFromString(str, strlen(str)));
}


static void Test6() {
  SetFlagsToDefault();
  int argc = 4;
  char* argv[] = { "Test5", "--int-flag", "0", "--float_flag" };
  CHECK_EQ(3, FlagList::SetFlagsFromCommandLine(&argc, argv, true));
  CHECK_EQ(4, argc);
}


static void Test6b() {
  SetFlagsToDefault();
  const char* str = "              --int-flag 0       --float_flag    ";
  CHECK_EQ(3, FlagList::SetFlagsFromString(str, strlen(str)));
}
