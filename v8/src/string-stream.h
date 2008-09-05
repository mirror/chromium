// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_STRING_STREAM_H_
#define V8_STRING_STREAM_H_

namespace v8 { namespace internal {


class StringAllocator {
 public:
  virtual ~StringAllocator() {}
  // Allocate a number of bytes.
  virtual char* allocate(unsigned bytes) = 0;
  // Allocate a larger number of bytes and copy the old buffer to the new one.
  // bytes is an input and output parameter passing the old size of the buffer
  // and returning the new size.  If allocation fails then we return the old
  // buffer and do not increase the size.
  virtual char* grow(unsigned* bytes) = 0;
};


// Normal allocator uses new[] and delete[].
class HeapStringAllocator: public StringAllocator {
 public:
  ~HeapStringAllocator() { DeleteArray(space_); }
  char* allocate(unsigned bytes);
  char* grow(unsigned* bytes);
 private:
  char* space_;
};


// Allocator for use when no new c++ heap allocation is allowed.
// Allocates all space up front and does no allocation while building
// message.
class NoAllocationStringAllocator: public StringAllocator {
 public:
  explicit NoAllocationStringAllocator(unsigned bytes);
  NoAllocationStringAllocator(char* memory, unsigned size);
  char* allocate(unsigned bytes) { return space_; }
  char* grow(unsigned* bytes);
 private:
  unsigned size_;
  char* space_;
};


class FmtElm {
 public:
  FmtElm(int value) : type_(INT) { data_.u_int_ = value; }  // NOLINT
  FmtElm(const char* value) : type_(C_STR) { data_.u_c_str_ = value; }  // NOLINT
  FmtElm(Object* value) : type_(OBJ) { data_.u_obj_ = value; }  // NOLINT
  FmtElm(Handle<Object> value) : type_(HANDLE) { data_.u_handle_ = value.location(); }  // NOLINT
  FmtElm(void* value) : type_(INT) { data_.u_int_ = reinterpret_cast<int>(value); }  // NOLINT
 private:
  friend class StringStream;
  enum Type { INT, C_STR, OBJ, HANDLE };
  Type type_;
  union {
    int u_int_;
    const char* u_c_str_;
    Object* u_obj_;
    Object** u_handle_;
  } data_;
};


class StringStream {
 public:
  explicit StringStream(StringAllocator* allocator):
    allocator_(allocator),
    capacity_(kInitialCapacity),
    length_(0),
    buffer_(allocator_->allocate(kInitialCapacity)) {
    buffer_[0] = 0;
  }

  ~StringStream() {
  }

  bool Put(char c);
  bool Put(String* str);
  bool Put(String* str, int start, int end);
  void Add(const char* format, Vector<FmtElm> elms);
  void Add(const char* format);
  void Add(const char* format, FmtElm arg0);
  void Add(const char* format, FmtElm arg0, FmtElm arg1);
  void Add(const char* format, FmtElm arg0, FmtElm arg1, FmtElm arg2);

  // Getting the message out.
  void OutputToStdOut();
  void Log();
  Handle<String> ToString();
  SmartPointer<char> ToCString();

  // Object printing support.
  void PrintName(Object* o);
  void PrintFixedArray(FixedArray* array, unsigned int limit);
  void PrintByteArray(ByteArray* ba);
  void PrintUsingMap(JSObject* js_object);
  void PrintPrototype(JSFunction* fun, Object* receiver);
  void PrintSecurityTokenIfChanged(Object* function);
  // NOTE: Returns the code in the output parameter.
  void PrintFunction(Object* function, Object* receiver, Code** code);

  // Reset the stream.
  void Reset() {
    length_ = 0;
    buffer_[0] = 0;
  }

  // Mentioned object cache support.
  void PrintMentionedObjectCache();
  static void ClearMentionedObjectCache();
#ifdef DEBUG
  static bool IsMentionedObjectCacheClear();
#endif


  static const int kInitialCapacity = 16;

 private:
  void PrintObject(Object* obj);

  StringAllocator* allocator_;
  unsigned capacity_;
  unsigned length_;  // does not include terminating 0-character
  char* buffer_;

  int space() const { return capacity_ - length_; }
  char* cursor() const { return buffer_ + length_; }

  DISALLOW_IMPLICIT_CONSTRUCTORS(StringStream);
};


} }  // namespace v8::internal

#endif  // V8_STRING_STREAM_H_
