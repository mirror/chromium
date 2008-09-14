#include "v9.h"
#include <stdio.h>
#include <time.h>
#include <string>

using namespace v9;

#define kIterations 50000000

#define STR(x) STR_HELPER(x)
#define STR_HELPER(x) #x

const char* start = 
  "(function () {"
  "  var a = { p: 4 };"
  "  var b = property_accessor_obj;"
  "  var c = function () { };"
  "  var d = function_obj;"
  "  for (var i = 0; i < " STR(kIterations) "; i++) {";

const char* end =
  "  }"
  "})()";

char* make_program(char* task, int repetitions) {
  std::string str = start;
  for (int i = 0; i < repetitions; i++)
    str += task;
  str += end;
  return strdup(str.c_str());
}

int time_execution(Environment& env, char* task, int repetitions) {
  LocalScope scope;
  char* program = make_program(task, repetitions);
  Local<Script> script = env.Compile(v8::V9::NewString(program));
  clock_t start = clock();
  script->Run(env);
  clock_t end = clock();
  return end - start;
}

int time_execution(Environment& env, char* task) {
  int time_one = time_execution(env, task, 1);
  int time_two = time_execution(env, task, 2);
  return time_two - time_one;
}

Handle<Data> read_p(const Environment& env, Handle<Object> self,
		    Handle<String> property, void* data) {
  return self;
}

void write_p(const Environment& env, Handle<Object> self, Handle<String> property,
	     Handle<Data> value, void* data) { }

Handle<Data> call_function(const Environment& env, Handle<Object> self,
			   Vector& args, void* data) {
  return self;
}

void configure_environment(Environment& env) {
  LocalScope scope;
  // Property accessor
  Local<FunctionTemplate> property_accessor_templ = FunctionTemplate::Create();
  property_accessor_templ->AddInstancePropertyAccessor(v8::V9::NewString("p"), read_p, write_p);
  Local<Object> obj = env.NewFunction(property_accessor_templ)->NewInstance(env);
  env.Global()->SetProperty(env, v8::V9::NewString("property_accessor_obj"), obj);
  // Function
  Local<FunctionTemplate> function_templ = FunctionTemplate::Create(call_function);
  Local<Function> function_obj = env.NewFunction(function_templ);
  env.Global()->SetProperty(env, v8::V9::NewString("function_obj"), function_obj);
}

struct TestCase {
  char* name;
  char* js_code;
  char* v9_code;
};

static const int kTestCaseCount = 3;
static TestCase kTestCases[kTestCaseCount] = {
// Name                 JS           V9
  {"Function calls",    "c();",      "d();"},
  {"Property read",     "a.p;",      "b.p;"},
  {"Property write",    "a.p = 0;",  "b.p = 0;"},
};

int main() {
  Environment env;
  configure_environment(env);
  for (int i = 0; i < kTestCaseCount; i++) {
    TestCase& test_case = kTestCases[i];
    printf("--- %-20s ---\n", test_case.name);
    int js_time = time_execution(env, test_case.js_code);
    printf("JavaScript:     %i ms.\n", js_time);
    int v9_time = time_execution(env, test_case.v9_code);
    printf("V9:             %i ms.\n", v9_time);
    float factor = static_cast<float>(v9_time) / js_time;
    printf("Factor:         %.1fx\n", factor);
  }
}
