#include "base/test/test_support_ios.h"  // nogncheck
#include "base/threading/thread_local.h"

class MyClass;

base::ThreadLocalPointer<MyClass>* MyGetTLSMessageLoop() {
  static auto* lazy_tls_ptr = new base::ThreadLocalPointer<MyClass>();
  return lazy_tls_ptr;
}

class MyClass {
public:
  void f() {
    MyGetTLSMessageLoop()->Set(this);
    if (this != MyGetTLSMessageLoop()->Get())
      asm("bkpt 0");
  }
};

void do_the_thing() {
  MyClass c;
  c.f();
}

int main(int argc, char* argv[]) {
  base::RunTestsFromIOSApp(argc, argv);
}
