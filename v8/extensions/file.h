#include <v8.h>

namespace v8 {

class FileNatives {
 public:
  static Handle<Value> FileExists(Arguments& args);
  static Handle<Value> FileRead(Arguments& args);
  static Handle<Value> FileIsDirectory(Arguments& args);
  static Handle<Value> FileList(Arguments& args);
  static Handle<Value> FileSeparator(Arguments& args);
};

}
