// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#include <ctype.h>

#include <signal.h>
#include <string>

#include "v8.h"

#include "execution.h"
#include "shell.h"

// Make SIGBREAK on Windows work as SIGQUIT on Linux
#ifdef WIN32
#define SIGQUIT SIGBREAK
#endif  // WIN32

namespace {

using v8::internal::PrintF;
using v8::internal::StringStream;

static std::string kPrompt("v8> ");
static const char kDebugPrompt[] = "v8 dbg> ";
static const char kHistoryFile[] = "/.v8_history";


static void PrintCommandUsage() {
  PrintF("Shell commands:\n");
  PrintF("  :h        display command help\n");
  PrintF("  :q        quit the shell\n");
#ifdef DEBUG
  PrintF("  :d <expr> evaluate <expr> and print structure of result\n");
#endif
  PrintF("  :r <file> read and evaluate the contents of the file\n");
}

}  // namespace


static void BreakHandler(int signum) {
  v8::Debug::DebugBreak();
  signal(SIGQUIT, &BreakHandler);
}


static void IntHandler(int signum) {
  v8::internal::StackGuard::Interrupt();
  signal(SIGINT, &IntHandler);
}


DECLARE_bool(shell_debugging);

namespace v8 { namespace internal {

DECLARE_bool(trace_debug_json);

using namespace v8;


// Reads a line from the prompt and returns a std::string after
// eliminating the trailing new line characters.
static std::string ReadPromptLine(std::string prompt, bool* eos) {
  char* content = v8::internal::ReadLine(prompt.c_str());
  if (content == NULL) {
    *eos = true;
    return std::string("");
  }
  std::string line(content);
  // Deallocate the allocated memory from ReadLine
  DeleteArray(content);
  if (!line.empty()) {
    // Remove trailing newline.
    int len = line.length();
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
      len--;
    }
    line = line.erase(len);
  }
  *eos = false;
  return line;
}


// Returns a std::string used for the prompt.
static std::string ComputePrompt(char* text) {
  return std::string("v8 dbg (") + text + ")> ";
}


// Retrieves the content from a file as a std::string.
static std::string GetFileContent(std::string file_name, bool* exists) {
  Vector<const char> content =
      v8::internal::ReadFile(file_name.c_str(), exists);
  std::string result = std::string(content.start());
  content.Dispose();
  return result;
}


void handle_debug_event(v8::DebugEvent event,
                        v8::Handle<v8::Object> exec_state,
                        v8::Handle<v8::Object> event_data,
                        v8::Handle<Value> data) {
  // Currently only handles break and exception events.
  if (event != v8::Break && event != v8::Exception) return;

  v8::TryCatch try_catch;

  if (FLAG_trace_debug_json) {
    Local<v8::String> fun_name = v8::String::New("toJSONProtocol");
    Local<v8::Function> fun = v8::Function::Cast(*event_data->Get(fun_name));
    Local<v8::Value> json = *fun->Call(event_data, 0, NULL);
    if (try_catch.HasCaught()) {
      Shell::PrintObject(try_catch.Exception());
      return;
    }
    Shell::PrintObject(json);
  }

  // Print the event details.
  Local<v8::String> details_fun_name = v8::String::New("details");
  Local<v8::Function> details_fun =
      v8::Function::Cast(*event_data->Get(details_fun_name));
  Local<v8::Value> details = *details_fun->Call(event_data, 0, NULL);
  if (try_catch.HasCaught()) {
    Shell::PrintObject(try_catch.Exception());
    return;
  }
  Shell::PrintObject(details);

  // Get the debug command processor.
  Local<v8::String> fun_name = v8::String::New("debugCommandProcessor");
  Local<v8::Function> fun = v8::Function::Cast(*exec_state->Get(fun_name));
  Local<v8::Object> cmd_processor =
      v8::Object::Cast(*fun->Call(exec_state, 0, NULL));
  if (try_catch.HasCaught()) {
    Shell::PrintObject(try_catch.Exception());
    return;
  }

  // Get the debugPrompt.
  Local<v8::String> debug_prompt_fun_name = v8::String::New("debugPrompt");
  Local<v8::Function> debug_prompt_fun =
      v8::Function::Cast(*event_data->Get(debug_prompt_fun_name));
  Local<v8::Value> debug_prompt = debug_prompt_fun->Call(event_data, 0, NULL);
  if (try_catch.HasCaught()) {
    Shell::PrintObject(try_catch.Exception());
    return;
  }
  v8::String::AsciiValue debug_prompt_ascii(debug_prompt);

  std::string prompt = ComputePrompt(*debug_prompt_ascii);
  bool eos = false;
  for (std::string command = ReadPromptLine(prompt, &eos); !eos;
       command = ReadPromptLine(prompt, &eos)) {
    // Ignore empty commands.
    if (command.empty()) continue;

    v8::Local<v8::String> fun_name;
    v8::Local<v8::Function> fun;
    // All the functions used below takes one argument.
    v8::Local<v8::Value> args[1];

    v8::TryCatch try_catch;

    // Invoke the JavaScript to convert the debug command line to a JSON
    // request, invoke the JSON request and convert the JSON respose to a text
    // representation.
    fun_name = v8::String::New("processDebugCommand");
    fun = v8::Function::Cast(*cmd_processor->Get(fun_name));
    args[0] = v8::String::New(command.c_str());
    v8::Local<v8::Value> result_val = fun->Call(cmd_processor, 1, args);
    if (try_catch.HasCaught()) {
      Shell::PrintObject(try_catch.Exception());
      continue;
    }
    v8::Local<v8::Object> result = v8::Object::Cast(*result_val);

    // Log the JSON request/response.
    if (FLAG_trace_debug_json) {
      Shell::PrintObject(result->Get(v8::String::New("request")));
      Shell::PrintObject(result->Get(v8::String::New("response")));
    }

    // Print the text result.
    v8::Local<v8::Value> text_result =
        result->Get(v8::String::New("text_result"));
    if (!text_result->IsUndefined()) {
      Shell::PrintObject(text_result);
    }

    // Return from debug event processing is VM should be running.
    if (result->Get(v8::String::New("running"))->ToBoolean()->Value()) {
      return;
    }
  }
}


void Shell::PrintObject(v8::Handle<v8::Value> result) {
  v8::HandleScope scope;
  v8::Local<v8::String> str_obj = result->ToDetailString();

  char* str = NULL;

  if (!str_obj.IsEmpty()) {
    int length = str_obj->Length();
    str = NewArray<char>(length + 1);
    str_obj->WriteAscii(str);
  }

  if (str == NULL) {
    PrintF("%s\n", "Failed to convert result to string");
  } else {
    PrintF("%s\n", str);
  }
  DeleteArray(str);
}


void Shell::Run(v8::Handle<v8::Context> context) {
  bool eos = false;
  for (std::string source = ReadPromptLine(kPrompt, &eos); !eos;
       source = ReadPromptLine(kPrompt, &eos)) {
    v8::HandleScope local;
    v8::Handle<v8::String> script_name = v8::Handle<v8::String>();
    if (source.empty()) continue;
    char command = '\0';
    unsigned int source_offset = 0;
    if (!source.empty() && source[0] == ':') {
      command = source[1];
      source_offset = 2;
      size_t len = source.length();
      while ((source_offset < len) && isspace(source[source_offset])) {
        source_offset++;
      }
      source = source.substr(source_offset);
    }
    bool used_command = false;
    switch (command) {
      case 'q':
        return;
      case 'h':
        PrintCommandUsage();
        continue;
      case 'r':
        used_command = true;
        if (!source.empty()) {
          bool exists;
          std::string file_name = source;
          source = GetFileContent(file_name, &exists);
          if (!exists || source.empty()) continue;
          script_name = v8::String::New(file_name.c_str());
        } else {
          PrintF("Missing file for '%c'\n", command);
          continue;
        }
        break;
      case 'd':
        if (source.empty()) continue;
        break;
      default:
        if (command != '\0') {
          PrintF("Unknown command '%c'\n", command);
          continue;
        }
    }
    v8::Context::Scope scope(context);
    v8::ScriptOrigin origin =
        v8::ScriptOrigin(script_name);
    v8::Local<v8::Script> code =
        v8::Script::Compile(v8::String::New(source.c_str()), &origin);
    if (code.IsEmpty()) continue;
    if (FLAG_shell_debugging) {
      signal(SIGINT, &IntHandler);
      signal(SIGQUIT, &BreakHandler);
    }
    v8::Local<v8::Value> result = code->Run();
    if (FLAG_shell_debugging) {
      signal(SIGINT, SIG_DFL);
      signal(SIGQUIT, SIG_DFL);
    }
    if (result.IsEmpty()) continue;
    switch (command) {
      case '\0': {
        if (!result->IsUndefined()) PrintObject(result);
        break;
      }
      default: {
        if (!used_command) PrintF("Unknown command '%c'\n", command);
        break;
      }
    }
  }
  PrintF("\n");
}


} }  // namespace v8::internal
