// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_CHAR_PREDICATES_H_
#define V8_CHAR_PREDICATES_H_

namespace v8 { namespace internal {

// Unicode character predicates as defined by ECMA-262, 3rd,
// used for lexical analysis.

inline bool IsCarriageReturn(uc32 c);
inline bool IsLineFeed(uc32 c);
inline bool IsDecimalDigit(uc32 c);
inline bool IsHexDigit(uc32 c);

struct IdentifierStart {
  static inline bool Is(uc32 c) {
    switch (c) {
      case '$': case '_': case '\\': return true;
      default: return unibrow::Letter::Is(c);
    }
  }
};


struct IdentifierPart {
  static inline bool Is(uc32 c) {
    return IdentifierStart::Is(c)
        || unibrow::Number::Is(c)
        || unibrow::CombiningMark::Is(c)
        || unibrow::ConnectorPunctuation::Is(c);
  }
};

} }  // namespace v8::internal

#endif  // V8_CHAR_PREDICATES_H_
