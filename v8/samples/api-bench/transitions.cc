#include <stdio.h>
#include <time.h>

#include <v9.h>

#define STR(s) _STR(s)
#define _STR(s) #s
#define ITERATIONS  50000000


// ---------------------------------------------------------------------
// C/C++ variables and functions that can be accessed from Java script
// ---------------------------------------------------------------------
int global_variable = 0;

void global_function(int i) {
  global_variable = i;
}

class C {
 public:
  C() : data_member_(0) {
  }

  virtual ~C() {
  }

  int get_data_member() {
    return data_member_;
  }

  void set_data_member(int value) {
    data_member_ = value;
  }

 private:
    int data_member_;
};

enum E {
  FORTYTWO = 42
};

// ---------------------------------------------------------------------
// Glue code for accessing the above C/C++ variables and functions
// Note: this code was auto-generated using a hacked version of Swig
// ---------------------------------------------------------------------
class V8MemberFunctionWrapper_C_get_data_member:
    public v8::FunctionCallHandler {
  virtual v8::Object HandleCall(const v8::Environment& env,
                                const v8::Object& self,
                                const v8::Vector& args);
};

class V8MemberFunctionWrapper_C_set_data_member:
    public v8::FunctionCallHandler {
  virtual v8::Object HandleCall(const v8::Environment& env,
                                const v8::Object& self,
                                const v8::Vector& args);
};

class V8DataMemberWrapper_C_data_member: public v8::PropertyAccessor {
 public:
  virtual v8::Object get(const v8::Environment& env, const v8::Object& obj);
  virtual void set(const v8::Environment& env,
                   const v8::Object& obj,
                   const v8::Object& value);
};

class V8WrapperFactory_C {
 public:
  static v8::Object Wrap(const v8::Environment& env, C* p);
  static C* UnWrap(const v8::Environment& env, v8::Object object);

 private:
  static v8::FunctionDescriptor v8WrapperFactory_C_desc_;
  static v8::Function* v8WrapperFactory_C_constructor_;
  static V8DataMemberWrapper_C_data_member
      v8WrapperFactory_C_data_member_accessor_;
  static V8MemberFunctionWrapper_C_get_data_member
      v8WrapperFactory_C_get_data_member_funcHandler_;
  static V8MemberFunctionWrapper_C_set_data_member
      v8WrapperFactory_C_set_data_member_funcHandler_;

  // Prevent any instances from being created
  V8WrapperFactory_C() {
  }

  virtual ~V8WrapperFactory_C() {
  }
};

class V8NewHandler_C: public v8::FunctionCallHandler {
  virtual v8::Object HandleCall(const v8::Environment& env,
                                const v8::Object& self,
                                const v8::Vector& args) {
    C* p = new C();
    return V8WrapperFactory_C::Wrap(env, p);
  }
};

static V8NewHandler_C new_C_handler;

static void V8FuncWrap_new_C(v8::FunctionDescriptor& func_desc) {
  func_desc.AddPrototypeMethod("new_C", &new_C_handler);
}

class V8DeleteHandler_C: public v8::FunctionCallHandler {
  virtual v8::Object HandleCall(const v8::Environment& env,
                                const v8::Object& self,
                                const v8::Vector& argv) {
    C* p = V8WrapperFactory_C::UnWrap(env, self);
    delete p;
    return env.GetUndefinedValue();
  }
};

static V8DeleteHandler_C delete_C_handler;

static void V8FuncWrap_delete_C(v8::FunctionDescriptor& func_desc) {
  func_desc.AddPrototypeMethod("delete_C", &delete_C_handler);
}

v8::Object V8DataMemberWrapper_C_data_member::get(const v8::Environment& env,
                                   const v8::Object& obj) {
  v8::ExternalWrapper wrapper =
      obj.GetInternalField(env, 0).AsExternalWrapper();
  C* c = static_cast<C*>(wrapper.GetValue(env));
  return env.NewNumber(c->get_data_member());
}

void V8DataMemberWrapper_C_data_member::set(const v8::Environment& env,
                             const v8::Object& obj,
                             const v8::Object& value) {
  v8::ExternalWrapper wrapper =
      obj.GetInternalField(env, 0).AsExternalWrapper();
  C* c = static_cast<C*>(wrapper.GetValue(env));
  int i = value.Int32Value(env);
  c->set_data_member(i);
}

v8::Object V8MemberFunctionWrapper_C_get_data_member::HandleCall(
    const v8::Environment& env,
    const v8::Object& self,
    const v8::Vector& args) {
  C* that = V8WrapperFactory_C::UnWrap(env, self);
  return env.NewNumber(static_cast<double>(that->get_data_member()));
}

v8::Object V8MemberFunctionWrapper_C_set_data_member::HandleCall(
    const v8::Environment& env,
    const v8::Object& self,
    const v8::Vector& args) {
  C* that = V8WrapperFactory_C::UnWrap(env, self);
  int value = args[0].Int32Value(env);
  that->set_data_member(value);
  return env.GetUndefinedValue();
}

v8::FunctionDescriptor V8WrapperFactory_C::v8WrapperFactory_C_desc_;
v8::Function* V8WrapperFactory_C::v8WrapperFactory_C_constructor_;
V8DataMemberWrapper_C_data_member
    V8WrapperFactory_C::v8WrapperFactory_C_data_member_accessor_;
V8MemberFunctionWrapper_C_get_data_member
    V8WrapperFactory_C::v8WrapperFactory_C_get_data_member_funcHandler_;
V8MemberFunctionWrapper_C_set_data_member
    V8WrapperFactory_C::v8WrapperFactory_C_set_data_member_funcHandler_;

v8::Object V8WrapperFactory_C::Wrap(const v8::Environment& env, C* p) {
  if (v8WrapperFactory_C_constructor_ == NULL) {
    v8WrapperFactory_C_desc_.AddPropertyAccessor("data_member",
        &v8WrapperFactory_C_data_member_accessor_);
    v8WrapperFactory_C_desc_.AddPrototypeMethod(
      "get_data_member", &v8WrapperFactory_C_get_data_member_funcHandler_);
    v8WrapperFactory_C_desc_.AddPrototypeMethod(
      "set_data_member", &v8WrapperFactory_C_set_data_member_funcHandler_);

    v8WrapperFactory_C_desc_.SetInternalFieldCount(1);

    v8WrapperFactory_C_constructor_ = new v8::Function(
        env.NewFunction(v8WrapperFactory_C_desc_).
            Globalize().AsFunction());
  }

  v8::Object object = v8WrapperFactory_C_constructor_->NewInstance(env);
  object.SetInternalField(env, 0, env.NewExternalWrapper(p));

  return object;
}

C* V8WrapperFactory_C::UnWrap(const v8::Environment& env,
                              v8::Object object) {
  v8::ExternalWrapper wrapper =
  object.GetInternalField(env, 0).AsExternalWrapper();
  return static_cast<C*>(wrapper.GetValue(env));
}

class V8FuncHandler_global_function:  public v8::FunctionCallHandler {
  virtual v8::Object HandleCall(const v8::Environment& env,
                                const v8::Object& self,
                                const v8::Vector& args) {
    int i = args[0].Int32Value(env);
    global_function(i);
    return env.GetUndefinedValue();
  }
};

static V8FuncHandler_global_function global_function_handler;

static void V8FuncWrap_global_function(v8::FunctionDescriptor& func_desc) {
  func_desc.AddPrototypeMethod("global_function", &global_function_handler);
}

class V8AccessWrapper_global_variable: public v8::PropertyAccessor {
 public:
  virtual v8::Object get(const v8::Environment& env, const v8::Object& obj);
  virtual void set(const v8::Environment& env,
                   const v8::Object& obj,
                   const v8::Object& value);
};

v8::Object V8AccessWrapper_global_variable::get(const v8::Environment& env,
                                                const v8::Object& obj) {
  return env.NewNumber(static_cast<double>(global_variable));
}

void V8AccessWrapper_global_variable::set(const v8::Environment& env,
                                          const v8::Object& obj,
                                          const v8::Object& value) {
  global_variable = value.Int32Value(env);
}

static V8AccessWrapper_global_variable v8VarWrap_global_variable_accessor_;

void V8VarWrap_global_variable(v8::FunctionDescriptor& func_desc) {
  func_desc.AddPropertyAccessor("global_variable",
                                &v8VarWrap_global_variable_accessor_);
}

class V8AccessWrapper_E_FORTYTWO: public v8::PropertyAccessor {
 public:
  virtual v8::Object get(const v8::Environment& env, const v8::Object& obj);
  virtual void set(const v8::Environment& env,
                   const v8::Object& obj,
                   const v8::Object& value) {
    // No setter for constant
  }
};

v8::Object V8AccessWrapper_E_FORTYTWO::get(const v8::Environment& env,
                                             const v8::Object& obj) {
  return env.NewNumber(static_cast<double>(FORTYTWO));
}

static V8AccessWrapper_E_FORTYTWO v8ConstWrap_E_FORTYTWO_accessor_;

void V8ConstWrap_E_FORTYTWO(v8::FunctionDescriptor& func_desc) {
  func_desc.AddPropertyAccessor("E_FORTYTWO",
                                &v8ConstWrap_E_FORTYTWO_accessor_);
}

void InitializeV8GlueCode(v8::FunctionDescriptor& desc) {
  V8ConstWrap_E_FORTYTWO(desc);
  V8VarWrap_global_variable(desc);
  V8FuncWrap_global_function(desc);
  V8FuncWrap_new_C(desc);
  V8FuncWrap_delete_C(desc);
}

// ---------------------------------------------------------------------
// Accessing a global variable and a global function
// ---------------------------------------------------------------------
static const char* javascript_source_run_access_globals =
"js_global_variable = 0\n"
"\n"
"function js_global_function(i) {\n"
"  js_global_variable = i\n"
"}\n"
"\n"
"function js_global_test(i) {\n"
"  js_global_function(i)\n"
"}\n"
"\n"
"function access_globals_javascript(iter) {\n"
"  for (var i = 0; i < iter; i++) {\n"
"    js_global_variable = 0\n"
"    js_global_test(42)\n"
"    if (js_global_variable != 42) {\n"
"      print('Failed!')\n"
"    }\n"
"  }\n"
"}\n"
"\n"
"access_globals_javascript(" STR(ITERATIONS) ")\n"
"\n";

static const char* javascript_source_run_access_globals_in_C =
"function global_test(i) {\n"
"  global_function(i)\n"
"}\n"
"\n"
"function access_globals_C(iter) {\n"
"  for (var i = 0; i < iter; i++) {\n"
"    global_variable = 0\n"
"    global_test(42)\n"
"    if (global_variable != 42) {\n"
"      print('Failed!')\n"
"    }\n"
"  }\n"
"}\n"
"\n"
"access_globals_C(" STR(ITERATIONS) ")\n"
"\n";

void global_test(int i) {
  global_function(i);
}

static void access_globals_C(int iters) {
  for (int i = 0; i < iters; i++) {
    global_variable = 0;
    global_test(42);
    if (global_variable != 42) {
      printf("Failed!\n");
    }
  }
}

// ---------------------------------------------------------------------
// Accessing a data member and a member function
// ---------------------------------------------------------------------
static const char* javascript_source_run_access_members =
"function set_data_member(i) {\n"
"  this.data_member = i\n"
"}\n"
"\n"
"function get_data_member() {\n"
"  return this.data_member\n"
"}\n"
"\n"
"function C() {\n"
"  this.data_member = 0\n"
"  this.set_data_member = set_data_member\n"
"  this.get_data_member = get_data_member\n"
"}\n"
"\n"
"function js_member_test(c, i) {\n"
"  c.set_data_member(i)\n"
"}\n"
"\n"
"function access_members_javascript(iter) {\n"
"  var c = new C()\n"
"  for (var i = 0; i < iter; i++) {\n"
"    c.set_data_member(0)\n"
"    js_member_test(c, 42)\n"
"    if (c.get_data_member() != 42) {\n"
"      print('Failed!')\n"
"    }\n"
"  }\n"
"}\n"
"\n"
"access_members_javascript(" STR(ITERATIONS) ")\n"
"\n";

static const char* javascript_source_run_access_members_in_C =
"function member_test(c, i) {\n"
"  c.data_member = i\n"
"}\n"
"\n"
"function access_members_C(iter) {\n"
"  var c = new_C();\n"
"  for (var i = 0; i < iter; i++) {\n"
"    c.data_member = 0\n"
"    member_test(c, 42)\n"
"    if (c.data_member != 42) {\n"
"      print('Failed!')\n"
"    }\n"
"  }\n"
"}\n"
"\n"
"access_members_C(" STR(ITERATIONS) ")\n"
"\n";

void member_test(C* c, int i) {
  c->set_data_member(i);
}

static void access_members_C(int iters) {
  C* c = new C();
  for (int i = 0; i < iters; i++) {
    c->set_data_member(0);
    member_test(c, 42);
    if (c->get_data_member() != 42) {
      printf("Failed!\n");
    }
  }
  delete c;
}

// ---------------------------------------------------------------------
// Timing the tests
// ---------------------------------------------------------------------
static void TimeScript(const v8::Environment& env,
                       const v8::Script& script,
                       const char* description,
                       const double base_time) {
  double start = clock();
  v8::Object result = script.Run(env);
  double time = clock() - start;
  fprintf(stdout,
          "\t%s: %g ms (factor: %g)\n",
          description,
          time / (CLOCKS_PER_SEC / 1000),
          time / base_time);
}

static double TimeFunction(void (*function)(int),
                           const char* description) {
  double start = clock();
  function(ITERATIONS);
  double time = clock() - start;
  fprintf(stdout,
          "\t%s: %g ms (factor 1)\n",
          description,
          time / (CLOCKS_PER_SEC / 1000));
  return time;
}

// ---------------------------------------------------------------------
// Executing the tests and comparing the results
// ---------------------------------------------------------------------

static v8::FunctionDescriptor global_func_desc;

int main(int argc, char* argv[]) {
  // V8 Initialization
  if (!v8::Initialize()) {
    fprintf(stderr, "Fatal Error: Couldn't initialize V8!\n");
    return 1;
  }

  InitializeV8GlueCode(global_func_desc);

  v8::Environment env(global_func_desc);
  if (!env.IsReady()) {
    fprintf(stderr, "Fatal Error: V8 environment not ready!\n");
    return 1;
  }

  // Compile scripts
  v8::EnvironmentLock lock(env);

  v8::Script script1 = env.Compile(javascript_source_run_access_globals, 0);
  if (script1.HasError()) {
    fprintf(stderr, "Fatal Error: Script contains errors!\n");
    return 1;
  }

  v8::Script script2 = env.Compile(javascript_source_run_access_globals_in_C, 0);
  if (script2.HasError()) {
    fprintf(stderr, "Fatal Error: Script contains errors!\n");
    return 1;
  }

  v8::Script script3 = env.Compile(javascript_source_run_access_members, 0);
  if (script3.HasError()) {
    fprintf(stderr, "Fatal Error: Script contains errors!\n");
    return 1;
  }

  v8::Script script4 = env.Compile(javascript_source_run_access_members_in_C, 0);
  if (script4.HasError()) {
    fprintf(stderr, "Fatal Error: Script contains errors!\n");
    return 1;
  }

  // Run comparisons
  fprintf(stdout, "Total time for %d iterations of ...\n", ITERATIONS);
  double base_time = TimeFunction(access_globals_C, "accessing globals in C");
  TimeScript(env, script1, "accessing globals in Javascript", base_time);
  TimeScript(env, script2, "accessing globals in C from Javascript", base_time);
  fprintf(stdout, "\n");

  fprintf(stdout, "Total time for %d iterations of ...\n", ITERATIONS);
  base_time = TimeFunction(access_members_C, "accessing members in C");
  TimeScript(env, script3, "accessing members in Javascript", base_time);
  TimeScript(env, script4, "accessing members in C from Javascript", base_time);

  // Shutdown
  v8::Dispose();
  return 0;
}
