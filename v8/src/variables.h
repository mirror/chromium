// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_VARIABLES_H_
#define V8_VARIABLES_H_

#include "zone.h"

namespace v8 { namespace internal {

class UseCount BASE_EMBEDDED {
 public:
  UseCount();

  // Inform the node of a "use". The weight can be used to indicate
  // heavier use, for instance if the variable is accessed inside a loop.
  void RecordRead(int weight);
  void RecordWrite(int weight);
  void RecordAccess(int weight);  // records a read & write
  void RecordUses(UseCount* uses);

  int nreads() const  { return nreads_; }
  int nwrites() const  { return nwrites_; }
  int nuses() const  { return nreads_ + nwrites_; }

  bool is_read() const  { return nreads() > 0; }
  bool is_written() const  { return nwrites() > 0; }
  bool is_used() const  { return nuses() > 0; }

#ifdef DEBUG
  void Print();
#endif

 private:
  int nreads_;
  int nwrites_;
};


// The AST refers to variables via VariableProxies - placeholders for the actual
// variables. Variables themselves are never directly referred to from the AST,
// they are maintained by scopes, and referred to from VariableProxies and Slots
// after binding and variable allocation.

class Variable: public ZoneObject {
 public:
  enum Mode {
    // User declared variables:
    VAR,       // declared via 'var', and 'function' declarations
    CONST,     // declared via 'const' declarations

    // Variables introduced by the compiler:
    DYNAMIC,   // always require dynamic lookup (we don't know the declaration)
    INTERNAL,  // like VAR, but not user-visible (may or may not be in a
               // context)
    TEMPORARY  // temporary variables (not user-visible), never in a context
  };

  // Printing support
  static const char* Mode2String(Mode mode);

  // Type testing & conversion
  Property* AsProperty();
  Variable* AsVariable();
  bool IsValidLeftHandSide() { return is_valid_LHS_; }

  // The source code for an eval() call may refer to a variable that is
  // in an outer scope about which we don't know anything (it may not
  // be the global scope). scope() is NULL in that case. Currently the
  // scope is only used to follow the context chain length.
  Scope* scope() const  { return scope_; }
  // If this assertion fails it means that some code has tried to
  // treat the special this variable as an ordinary variable with
  // the name "this".
  Handle<String> name() const  { return name_; }
  Mode mode() const  { return mode_; }
  bool is_accessed_from_inner_scope() const  {
    return is_accessed_from_inner_scope_;
  }
  UseCount* var_uses()  { return &var_uses_; }
  UseCount* obj_uses()  { return &obj_uses_; }

  bool IsVariable(Handle<String> n) {
    return !is_this() && name().is_identical_to(n);
  }

  bool is_global() const;
  bool is_this() const { return is_this_; }

  Expression* rewrite() const  { return rewrite_; }
  Slot* slot() const;

 private:
  Variable(Scope* scope, Handle<String> name, Mode mode, bool is_valid_LHS,
      bool is_this);

  Scope* scope_;
  Handle<String> name_;
  Mode mode_;
  bool is_valid_LHS_;
  bool is_this_;

  // Usage info.
  bool is_accessed_from_inner_scope_;  // set by variable resolver
  UseCount var_uses_;  // uses of the variable value
  UseCount obj_uses_;  // uses of the object the variable points to

  // Code generation.
  // rewrite_ is usually a Slot or a Property, but maybe any expression.
  Expression* rewrite_;

  friend class VariableProxy;
  friend class Scope;
  friend class LocalsMap;
  friend class AstBuildingParser;
};


} }  // namespace v8::internal

#endif  // V8_VARIABLES_H_
