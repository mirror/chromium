typedef __SIZE_TYPE__ size_t;
typedef int int32_t;
typedef long long int64_t;

#if 1 // SUSPICIOUS_MODE

#define NOT_SUSPICIOUS(...) \
    namespace NOT_SUSPICIOUS { __VA_ARGS__ }

#define VALUES_BEFORE size_t values[5]
#define VALUES_BEFORE_1 size_t values[4]
#define VALUES_AFTER size_t more_values[1]

namespace NOT_suspicious {

struct Int64 {
  VALUES_BEFORE_1;
  int64_t NOT_suspicious;
  VALUES_AFTER;
};

struct VirtualTable {
  virtual ~VirtualTable() {}
  VALUES_BEFORE;
};
struct Vtbl: public VirtualTable {
  VALUES_AFTER;
};

}  // not_suspicious

namespace suspicious {

struct Pointer;

struct Pointer {
  VALUES_BEFORE;
  void* suspicious;
  VALUES_AFTER;
};

struct Short {
  VALUES_BEFORE;
  short suspicious;
  short suspicious2;
  VALUES_AFTER;
};

struct Array {
  VALUES_BEFORE;
  char suspicious[3];
  VALUES_AFTER;
};

template <class T>
struct unique_ptr {
  typedef T value_type;
  typedef value_type* ptr_type;
  ptr_type ptr;
};
struct UniquePtr {
  VALUES_BEFORE;
  unique_ptr<char> suspicious;
  VALUES_AFTER;
};

struct NestedPointer {
  struct Inner {
    bool flag;
    void* pointer;
  };

  VALUES_BEFORE_1;
  Inner suspicious;
  VALUES_AFTER;
};

// 5 suspicious cases

struct BaseValuesBefore {
  VALUES_BEFORE;
};
struct InheritedValues: BaseValuesBefore {
  int suspicious;
  VALUES_AFTER;
};

struct BaseSuspicious {
  void* suspicious;
};
struct InheritedSuspicious: BaseValuesBefore, BaseSuspicious {
  VALUES_AFTER;
};

template <class T>
struct tree_node {
  T value;
  VALUES_AFTER;
};
struct TreeValue {
  VALUES_BEFORE;
  void* suspicious;
};
struct Tree {
  Tree(): root(new tree_node<TreeValue>()) {}

  tree_node<TreeValue>* root;
};

// 8 suspicious cases

}

#else // DEVELOPMENT_MODE

struct Pointer {
  void* value;
};

enum Color {
  Red,
  Green,
  Yellow
};

union Union {
  float f;
  int32_t i;
};

namespace simple {

struct Object {
  size_t count;
  Pointer ptr1;
  Pointer ptr2;
  const char* name;
  unsigned some_flags: 3;
  unsigned:3;
  unsigned more_flags: 2;
};

struct ContainerObject {
  Object object;
  char data[10];
  union {
    int i;
    char c;
  } u;
  Color color;
};

struct Small {
  short x;
  char y;
};

struct Medium {
  Small s1;
  Small s2;
  void* ptr;
};

struct Large {
  Medium m1;
  Medium m2;
};

}  // simple

namespace inheritance {

struct BaseObject {
  BaseObject(const char* name): name(name) {}
  virtual ~BaseObject() {}

  const char* const name;
  char buffer[100];
  Pointer ptr[5];
};

struct DerivedObject: BaseObject {
  int64_t time;
  const char* accessCode;
};

struct EmptyBase_0 { virtual int f0() { return 0; } };
struct EmptyBase_1 { virtual int f1() { return 1; } };
struct NonEmptyBase_2 { virtual int f2() { return 2; } int payload[4]; };
struct EmptyBase_3 { virtual int f3() { return 3; } };
struct EmptyBase_4 { virtual int f4() { return 4; } };
struct Empty: EmptyBase_0, EmptyBase_1, NonEmptyBase_2, EmptyBase_3, EmptyBase_4 {};

struct VirtualBase {
  virtual ~VirtualBase() {}
  int data;
};

struct Virtual_1: virtual VirtualBase {};
struct Virtual_2: virtual VirtualBase {
  char ch[3];
};
struct Virtual_X: virtual VirtualBase, Virtual_1, Virtual_2 {
  bool flag;
};
struct Virtual_Y: Virtual_1, Virtual_2 {
  bool flag;
};
struct Virtual_Z: Virtual_1, Virtual_2, virtual VirtualBase {
  bool flag;
  char ch;
};

}  // inheritance

namespace templates {

template <class C>
struct T1 {
  bool flag;
  C value;
  size_t count;
};

struct T2 {
  T2() : t() {
    auto x = [&]() {
      t.flag = true;
    };
  }
  T1<int> t;
};

template class T1<char>;

template <class C>
struct T3 {
  struct Plain {
    int data;
    bool flag;
  };
};

template class T3<int>;

}  // templates

#endif