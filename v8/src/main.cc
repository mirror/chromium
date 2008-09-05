// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>


#ifdef WIN32
// Platform specific code for Win32.
#ifndef WIN32_LEAN_AND_MEAN
// WIN32_LEAN_AND_MEAN implies NOCRYPT and NOGDI.
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef NOKERNEL
#define NOKERNEL
#endif
#ifndef NOUSER
#define NOUSER
#endif
#ifndef NOSERVICE
#define NOSERVICE
#endif
#ifndef NOSOUND
#define NOSOUND
#endif
#ifndef NOMCX
#define NOMCX
#endif

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")  // force linkage with winmm.

#undef VOID
#undef DELETE
#undef IN
#undef THIS
#undef CONST
#undef NAN
#undef GetObject
#undef CreateMutex
#undef CreateSemaphore

#endif

#include <signal.h>
#include <string>
#include <map>

#include "v8.h"

#include "api.h"
#include "debug.h"
#include "shell.h"
#include "serialize.h"
#include "snapshot.h"
#include "platform.h"

// use explicit namespace to avoid clashing with types in namespace v8
namespace i = v8::internal;
using namespace v8;

DEFINE_bool(shell_debugging, false, "interactive debugging in the shell");
DEFINE_int(iterations, 1, "number of script execution iterations");
DEFINE_bool(h, false, "print this message");
DEFINE_bool(execute, true, "execute script");
DEFINE_bool(shell, false, "interactive shell");
DEFINE_bool(standardize_messages, false, "standardize messages");
DEFINE_string(e, NULL, "execute an expression");
// The -f flag is for compatibility with SunSpider's -f <filename>.
// We just make the -f be a flag and swallow it.
DEFINE_bool(f, false, "load a js source file");
DEFINE_bool(stack_trace_on_fault, true,
            "print a stack trace on a memory fault");
DEFINE_bool(version, false, "print the current version");
DEFINE_bool(counters, false, "dump the counters on exit");
DEFINE_string(save_counters, NULL, "store counters in a memory-mapped file");
DEFINE_string(expose_natives_as, NULL, "expose natives in global object");
DEFINE_string(expose_debug_as, NULL, "expose debug in global object");
DEFINE_bool(win32_debug_popup, false, "show debug popup on Windows");
#ifndef USE_SNAPSHOT
DEFINE_string(write_snapshot, NULL, "write snapshot to file and exit");
DEFINE_string(snapshot_file, NULL, "start up from the snapshot file");
#endif
#ifdef DEBUG
DEFINE_bool(print_on_exit, false, "print objects on exit");
#endif

static const unsigned int kMaxCounters = 256;


// A single counter in a counter collection.
class Counter {
 public:
  static const int kMaxNameSize = 64;
  int32_t* Bind(const wchar_t* name) {
    int i;
    for (i = 0; i < kMaxNameSize - 1 && name[i]; i++)
      name_[i] = static_cast<char>(name[i]);
    name_[i] = '\0';
    return &counter_;
  }
 private:
  int32_t counter_;
  uint8_t name_[kMaxNameSize];
};


// A set of counters and associated information.  An instance of this
// class is stored directly in the memory-mapped counters file if
// the --save-counters options is used
class CounterCollection {
 public:
  CounterCollection() {
    magic_number_ = 0xDEADFACE;
    max_counters_ = kMaxCounters;
    max_name_size_ = Counter::kMaxNameSize;
    counters_in_use_ = 0;
  }
  Counter* GetNextCounter() {
    if (counters_in_use_ == kMaxCounters) return NULL;
    return &counters_[counters_in_use_++];
  }
 private:
  uint32_t magic_number_;
  uint32_t max_counters_;
  uint32_t max_name_size_;
  uint32_t counters_in_use_;
  Counter counters_[kMaxCounters];
};


// We statically allocate a set of local counters to be used if we
// don't want to store the stats in a memory-mapped file
static CounterCollection local_counters;
static CounterCollection* counters = &local_counters;
// The counters file; it is only non-null if the --save-counters
// option is used
static i::OS::MemoryMappedFile* counters_file = NULL;


typedef std::map<std::wstring, int*> CounterMap;
typedef std::map<std::wstring, int*>::iterator CounterMapIterator;
static CounterMap counter_table_;

// Callback receiver when v8 has a counter to track.
static int* CounterCallback(const wchar_t* name) {
  std::wstring counter = name;
  // See if this counter name is already known.
  if (counter_table_.find(counter) != counter_table_.end())
    return counter_table_[counter];

  Counter* ctr = counters->GetNextCounter();
  if (ctr == NULL) return NULL;
  int* ptr = ctr->Bind(name);
  counter_table_[counter] = ptr;
  return ptr;
}

// Prints the counters to stdout.
static void DumpCounters() {
  const int table_width = 34;
  CounterMapIterator it = counter_table_.begin();
  while (it != counter_table_.end()) {
    std::wstring name = it->first;
    int* value = it->second;
    ::printf("%ls", name.c_str());
    for (int i = table_width - name.length(); i >= 0; i--) ::printf(" ");
    ::printf("%d\n", *value);
    it++;
  }
}

static void HandleMessage(v8::Handle<Message> message, v8::Handle<Value>) {
  v8::HandleScope scope;
  v8::String::AsciiValue message_str(message->Get());
  v8::Handle<Value> data = message->GetSourceData();
  if (!data->IsUndefined()) {
    v8::String::AsciiValue file_name(data);
    int line_number = message->GetLineNumber();
    if (line_number == -1) {
      i::PrintF("%s: %s\n", *file_name, *message_str);
    } else {
      i::PrintF("%s:%i: %s\n", *file_name, line_number, *message_str);
      Local<Value> source_line_obj = message->GetSourceLine();
      if (source_line_obj->IsString()) {
        v8::String::AsciiValue source_line(source_line_obj);
        char* underline = message->GetUnderline(*source_line, '^');
        i::PrintF("%s\n%s\n", *source_line, underline);
        i::DeleteArray(underline);
      }
    }
  } else {
    i::PrintF("%s\n", *message_str);
  }
  if (!FLAG_standardize_messages)
    Message::PrintCurrentStackTrace(stdout);
}


static void HandleFatalError(const char* location, const char* message) {
  i::PrintF("\n%s: %s\n", (location == NULL) ? "FATAL" : location, message);
  // NOTE: We exit with error code 5 here, because that is apparently
  // what SpiderMonkey does.
  exit(5);
}


v8::Handle<Value> ExecuteString(v8::Handle<v8::Context> context,
    i::Vector<const char> script, i::Vector<const char> file_name) {
  // Compile and run script
  v8::Handle<String> file_name_str;
  if (!file_name.is_empty()) {
    file_name_str = v8::String::New(file_name.start(), file_name.length());
  }
  v8::Handle<String> source =
      v8::String::New(script.start(), script.length());
  v8::ScriptOrigin origin =
      v8::ScriptOrigin(file_name_str);
  v8::ScriptData* pre_data =
      v8::ScriptData::PreCompile(script.start(), script.length());
  v8::Handle<Script> code = Script::Compile(source, &origin, pre_data);
  delete pre_data;

  if (code.IsEmpty() || !FLAG_execute) return v8::Handle<Value>();

  // Execute script
  return code->Run();
}


bool RunScript(v8::Handle<v8::Context> context, i::Vector<char> file_name) {
  // Read script from file
  bool exists;
  i::Vector<const char> script = i::ReadFile(file_name.start(), &exists);
  if (script.is_empty()) return true;

  // Find the base file name
  char* base_file_name = file_name.start();
  if (FLAG_standardize_messages) {
    for (char* p = base_file_name; *p; p++) {
      if (*p == '/' || *p == '\\') base_file_name = p + 1;
    }
  }

  v8::Handle<Value> result = ExecuteString(context, script,
      i::CStrVector(base_file_name));
  script.Dispose();
  return !result.IsEmpty();
}


static void SegmentationFaultHandler(int sig) {
  signal(SIGSEGV, SegmentationFaultHandler);
  signal(SIGILL, SegmentationFaultHandler);
  signal(SIGFPE, SegmentationFaultHandler);
  i::PrintF("\n");
  i::PrintF("***********************************\n");
  i::PrintF("Caught signal %d\n", sig);
  i::PrintF("***********************************\n");
  i::Top::PrintStack();
  i::OS::Abort();
}


static void RegisterFaultHandler() {
  signal(SIGSEGV, SegmentationFaultHandler);
  signal(SIGILL, SegmentationFaultHandler);
  signal(SIGFPE, SegmentationFaultHandler);
}


int RunMain(v8::Handle<v8::Context> context, int argc, char** argv) {
#ifndef WIN32
  if (FLAG_standardize_messages) setenv("LANG", "en_US", true);
#endif
  v8::V8::SetFatalErrorHandler(HandleFatalError);
  v8::V8::AddMessageListener(HandleMessage);

  // Register debug event handler.
  if (FLAG_shell_debugging) {
    v8::Debug::AddDebugEventListener(i::handle_debug_event);
  }

  if (FLAG_stack_trace_on_fault) {
    RegisterFaultHandler();
  }

  if (FLAG_e) {
    // Create an environment for executing JavaScript code.
    v8::HandleScope handle_scope;
    v8::Handle<Value> result =
        ExecuteString(context, i::CStrVector(FLAG_e),
                      i::Vector<const char>::empty());
    if (result.IsEmpty()) return 3;
    else if (!result->IsUndefined()) i::Shell::PrintObject(result);
  }

  for (int it = 0; it < FLAG_iterations; it++) {
    for (int i = 1; i < argc; i++) {
      if (!RunScript(context, i::MutableCStrVector(argv[i]))) {
        // NOTE: We return 3 as the error code here, because that is
        // apparently what SpiderMonkey does.
        return 3;
      }
    }
  }

  // We automatically run the shell if there are no file arguments and
  // no --exec flag.  And, of course, if the --shell flag has been
  // specified.
  bool auto_run_shell = !FLAG_e && argc == 1;
  if (FLAG_shell || auto_run_shell)
    i::Shell::Run(context);

  return 0;
}


#ifdef WIN32

int PlatformRunMain(v8::Handle<v8::Context> context, int argc, char** argv) {
  int rv = -1;

  static const int kTimerResolution = 1;
  timeBeginPeriod(kTimerResolution);

  // Run with or without debug popup.
  if (FLAG_win32_debug_popup) {
    rv = RunMain(context, argc, argv);
  } else {
    // When running on the build bot, it is important that crashes don't trigger
    // UI to popup on the build machine.  Turn off pop-up error handling and
    // catch exceptions that dump to the console.
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOALIGNMENTFAULTEXCEPT |
                 SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    // Also send the Visual Studio C++ Runtime Library assertions to the
    // debugger's output window and stdout instead of displaying a dialog.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

    __try {
      rv = RunMain(context, argc, argv);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
      printf("Exception thrown with code 0x%x\n", GetExceptionCode());
    }
  }

  // Reset the timer resolution before returning.
  timeEndPeriod(kTimerResolution);
  return rv;
}
#else
int PlatformRunMain(v8::Handle<v8::Context> context, int argc, char** argv) {
  return RunMain(context, argc, argv);
}
#endif


static void SetUpCounters() {
  if (FLAG_save_counters) {
    counters_file = i::OS::MemoryMappedFile::create(FLAG_save_counters,
      sizeof(CounterCollection), &local_counters);
    if (counters_file == NULL) {
      printf("Could not open counters store %s\n", FLAG_save_counters);
      exit(1);
    }
    void* memory = counters_file->memory();
    if (memory == NULL) {
      printf("Error memory mapping counters\n");
    } else {
      counters = static_cast<CounterCollection*>(memory);
    }
  }
  v8::V8::SetCounterFunction(CounterCallback);
}


static void CloseCounters() {
  if (counters_file)
    delete counters_file;
}


static void PrintVersion() {
  v8::VersionInfo info;
  v8::V8::GetVersion(&info);
  ::printf("V8 JavaScript engine %i.%i.%i.%i\n", info.major, info.minor,
           info.build_major, info.build_minor);
}


static v8::Persistent<v8::Context> NewContext() {
  const int kExtensionCount = 5;
  const char* extension_list[kExtensionCount] = { "v8/print",
                                                  "v8/load",
                                                  "v8/quit",
                                                  "v8/version",
                                                  "v8/gc" };
  v8::ExtensionConfiguration extensions(kExtensionCount, extension_list);
  return v8::Context::New(&extensions);
}


int main(int argc, char** argv) {
  // Copy the arguments, because SetFlagsFromCommandLine is destructive.
  char** arg_array = i::NewArray<char*>(argc);
  memcpy(arg_array, argv, argc * sizeof(arg_array[0]));
  int arg_count = argc;
  // Print the usage if an error occurs when parsing the command line
  // flags or if the help flag is set.
  if (i::FlagList::SetFlagsFromCommandLine(&argc, argv, true) > 0 || FLAG_h) {
    ::printf("Usage: %s [flag | file] ...\n", argv[0]);
    i::FlagList::Print(NULL, false);
    return 1;
  }

  if (FLAG_version) {
    PrintVersion();
    return 0;
  }

  SetUpCounters();
  v8::HandleScope scope;
  v8::Persistent<v8::Context> context;
#ifdef USE_SNAPSHOT
  i::Serializer::disable();
  i::Snapshot::Initialize();
#else
  if (FLAG_write_snapshot) {
    i::Snapshot::DisableInternal();
  } else {
    i::Serializer::disable();
  }
  i::Snapshot::Initialize(FLAG_snapshot_file);
#endif
  context = NewContext();
  context->Enter();
  // Overwrite arguments from the snapshot if explicitly given on the
  // command line.
  i::FlagList::SetFlagsFromCommandLine(&arg_count, arg_array, true);
  i::DeleteArray(arg_array);

#ifndef USE_SNAPSHOT
  if (FLAG_write_snapshot) {
    return i::Snapshot::WriteToFile(FLAG_write_snapshot) ? 0 : -1;
  }
#endif

  // Expose the natives in global if a name for it is specified.
  if (FLAG_expose_natives_as != NULL) {
    if (strlen(FLAG_expose_natives_as) != 0) {
      i::Handle<i::JSGlobalObject> global =
          i::Handle<i::JSGlobalObject>::cast(
              v8::Utils::OpenHandle(*context->Global()));
      i::Handle<i::String> natives_string =
          i::Factory::LookupAsciiSymbol(FLAG_expose_natives_as);
      i::SetProperty(global, natives_string,
                     i::Handle<i::JSObject>(global->builtins()), DONT_ENUM);
    }
  }

  // Expose the debug global object in global if a name for it is specified.
  if (FLAG_expose_debug_as != NULL) {
    if (strlen(FLAG_expose_debug_as) != 0) {
      i::Debug::Load();
      i::Handle<i::JSGlobalObject> debug_global =
          i::Handle<i::JSGlobalObject>(
              i::JSGlobalObject::cast(i::Debug::debug_context()->global()));
      i::Handle<i::String> debug_string =
          i::Factory::LookupAsciiSymbol(FLAG_expose_debug_as);
      i::Handle<i::JSGlobalObject> global =
          i::Handle<i::JSGlobalObject>::cast(
              v8::Utils::OpenHandle(*context->Global()));
      i::SetProperty(global, debug_string,
                     i::Handle<i::JSObject>(debug_global), DONT_ENUM);

      // Set the security token for the debugger global object to the same as
      // the shell global object to allow calling between these (otherwise
      // exposing debug global object dosen't make much sense).
      debug_global->set_security_token(global->security_token());
    }
  }

  int err_code = PlatformRunMain(context, argc, argv);

  context->Exit();
  context.Dispose();

#ifdef DEBUG
  if (FLAG_print_on_exit) {
    i::Heap::Print();
  }
#endif

  if (FLAG_counters) {
    DumpCounters();
  }

  v8::internal::V8::TearDown();
  CloseCounters();
  return err_code;
}
