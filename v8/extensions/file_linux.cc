#include "file.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

using namespace v8;


/* native: v8::FileNatives::FileExists */
Handle<Value> v8::FileNatives::FileExists(Arguments& args) {
  String::AsciiValue name(args[0]);
  struct stat s;
  int i = stat(*name, &s);
  return Boolean::New(i == 0);
}


/* native: v8::FileNatives::FileRead */
Handle<Value> v8::FileNatives::FileRead(Arguments& args) {
  String::AsciiValue value(args[0]);
  FILE* file = fopen(*value, "rb");
  if (file == NULL)
    return ThrowException(Exception::Error(String::New("Cannot open file")));

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  rewind(file);

  char* result = new char[size + 1];
  result[size] = '\0';
  for (int i = 0; i < size; ) {
    int read = fread(&result[i], 1, size - i, file);
    if (read <= 0) {
      fclose(file);
      DeleteArray(result);
      return ThrowException(Exception::Error(String::New("Problem while reading file")));
    }
    i += read;
  }
  fclose(file);
  Handle<String> result_obj = String::New(result);
  DeleteArray(result);
  return result_obj;
}


/* native: v8::FileNatives::FileIsDirectory */
Handle<Value> v8::FileNatives::FileIsDirectory(Arguments& args) {
  String::AsciiValue name(args[0]);
  struct stat buf;
  int i = stat(*name, &buf);
  if (i != 0) return False();
  return Boolean::New(S_ISDIR(buf.st_mode));
}


/* native: v8::FileNatives::FileList */
Handle<Value> v8::FileNatives::FileList(Arguments& args) {
  String::AsciiValue name(args[0]);
  DIR* dp = opendir(*name);
  if (dp == NULL)
    return ThrowException(Exception::Error(String::New("Cannot list directory")));
  struct dirent *dirp;
  Handle<Array> result = Array::New();
  while ((dirp = readdir(dp)) != NULL)
    result->Set(Integer::New(result->Length()), String::New(dirp->d_name));
  closedir(dp);
  return result;
}

/* native: v8::FileNatives::FileSeparator */
Handle<Value> v8::FileNatives::FileSeparator(Arguments& args) {
  return String::New("/");
}
