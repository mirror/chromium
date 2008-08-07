// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#include <stdio.h>
#include <string.h>

#include "api/v8api"
#include "checks.h"

using namespace ::v8;

int global_index = 0;

class Snorkel {
 public:
  Snorkel() { index_ = global_index++; }
  int index_;
};

class WhammyPropertyHandler : public NamedPropertyHandler {
 public:
  WhammyPropertyHandler() {
    cursor_ = 0;
    for (int i = 0; i < kObjectCount; i++)
      objects_[i] = NULL;
  }

  virtual Object get(const Environment& env,
                     const Object& self,
                     const Object& key,
                     bool* found) {
    V8Object* prev_ptr = objects_[cursor_];
    Object obj = env.NewObject();
    Object global = obj.Globalize();
    if (prev_ptr != NULL) {
      Object prev(prev_ptr);
      prev.SetProperty(env, "next", obj);
      prev.MakeWeak(new Snorkel());
      objects_[cursor_] = NULL;
    }
    objects_[cursor_] = global.value();
    cursor_ = (cursor_ + 1) % kObjectCount;
    *found = true;
    return getScript(env).Run(env);
  }
  Script getScript(const Environment& env) {
    if (script_.HasError()) {
      script_ = env.Compile("({}).blammo", NULL).Globalize();
    }
    return script_;
  }
 private:
  static const int kObjectCount = 256;
  int cursor_;
  V8Object* objects_[kObjectCount];
  Script script_;
};

static void WeakReferenceHandler(V8Object* obj_ptr, void* data) {
  Snorkel* snorkel = reinterpret_cast<Snorkel*>(data);
  printf("%i\n", snorkel->index_);
  delete snorkel;
  Object obj(obj_ptr);
  obj.UnGlobalize();
}

static void TestStress() {
  FunctionDescriptor interceptor_descriptor;
  WhammyPropertyHandler whammy_handler;
  interceptor_descriptor.SetNamedPropertyHandler(&whammy_handler);
  Environment env;
  EnvironmentLock lock(env);
  env.SetWeakReferenceHandler(WeakReferenceHandler);
  CHECK(env.IsReady());
  Object interceptor = interceptor_descriptor.NewInstance(env);
  env.GetGlobalObject().SetProperty(env, "whammy", interceptor);
  char* code =
    "var last;"
    "for (var i = 0; i < 10000; i++) {"
    "  var obj = whammy.length;"
    "  if (last) last.next = obj;"
    "  last = obj;"
    "}"
    "4";
  Script script = env.Compile(code, NULL);
  Object result = script.Run(env);
  CHECK_EQ(4, result.NumberValue(env));
}
