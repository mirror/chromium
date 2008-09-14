// Copyright 2007-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_DISASM_H_
#define V8_DISASM_H_

namespace disasm {

typedef unsigned char byte;

// Interface and default implementation for converting addresses and
// register-numbers to text.  The default implementation is machine
// specific.
class NameConverter {
 public:
  virtual ~NameConverter() {}
  virtual const char* NameOfCPURegister(int reg) const;
  virtual const char* NameOfXMMRegister(int reg) const;
  virtual const char* NameOfAddress(byte* addr) const;
  virtual const char* NameOfConstant(byte* addr) const;
  virtual const char* NameInCode(byte* addr) const;
};


// A generic Disassembler interface
class Disassembler {
 public:
  // Uses default NameConverter.
  Disassembler();

  // Caller deallocates converter.
  explicit Disassembler(const NameConverter& converter);

  virtual ~Disassembler();

  // Writes one disassembled instruction into 'buffer' (0-terminated).
  // Returns the length of the disassembled machine instruction in bytes.
  int InstructionDecode(char* buffer, const int buffer_size, byte* instruction);

  // Write disassembly into specified file 'f' using specified NameConverter
  // (see constructor).
  static void Disassemble(FILE* f, byte* begin, byte* end);
 private:
  const NameConverter& converter_;
};

}  // namespace disasm

#endif  // V8_DISASM_H_
