// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#include "v8.h"

namespace v8 { namespace internal {


void DescriptorWriter::Write(Descriptor* desc) {
  ASSERT(desc->key_->IsSymbol());
  descriptors_->Set(pos_, desc);
  advance();
}


void DescriptorWriter::WriteFrom(DescriptorReader* reader) {
  Descriptor desc;
  reader->Get(&desc);
  Write(&desc);
}


#ifdef DEBUG
void LookupResult::Print() {
  if (!IsValid()) {
    PrintF("Not Found\n");
    return;
  }

  PrintF("LookupResult:\n");
  PrintF(" -cacheable = %s\n", IsCacheable() ? "true" : "false");
  PrintF(" -attributes = %x\n", GetAttributes());
  switch (type()) {
    case NORMAL:
      PrintF(" -type = normal\n");
      PrintF(" -entry = %d", GetDictionaryEntry());
      break;
    case MAP_TRANSITION:
      PrintF(" -type = map transition\n");
      PrintF(" -map:\n");
      GetTransitionMap()->Print();
      PrintF("\n");
      break;
    case CONSTANT_FUNCTION:
      PrintF(" -type = constant function\n");
      PrintF(" -function:\n");
      GetConstantFunction()->Print();
      PrintF("\n");
      break;
    case FIELD:
      PrintF(" -type = field\n");
      PrintF(" -index = %d", GetFieldIndex());
      PrintF("\n");
      break;
    case CALLBACKS:
      PrintF(" -type = call backs\n");
      PrintF(" -callback object:\n");
      GetCallbackObject()->Print();
      break;
    case INTERCEPTOR:
      PrintF(" -type = lookup interceptor\n");
      break;
    case CONSTANT_TRANSITION:
      PrintF(" -type = constant property transition\n");
      break;
  }
}


void Descriptor::Print() {
  PrintF("Descriptor ");
  GetKey()->ShortPrint();
  PrintF(" @ ");
  GetValue()->ShortPrint();
  PrintF(" %d\n", GetDetails().index());
}


#endif


} }  // namespace v8::internal
