// Copyright 2006-2008 Google Inc. All Rights Reserved.
// <<license>>

#ifndef V8_PRETTYPRINTER_H_
#define V8_PRETTYPRINTER_H_

#include "ast.h"

namespace v8 { namespace internal {

#ifdef DEBUG

class PrettyPrinter: public Visitor {
 public:
  PrettyPrinter();
  virtual ~PrettyPrinter();

  // The following routines print a node into a string.
  // The result string is alive as long as the PrettyPrinter is alive.
  const char* Print(Node* node);
  const char* PrintExpression(FunctionLiteral* program);
  const char* PrintProgram(FunctionLiteral* program);

  // Print a node to stdout.
  static void PrintOut(Node* node);

  // Individual nodes
#define DEF_VISIT(type)                         \
  virtual void Visit##type(type* node);
  NODE_LIST(DEF_VISIT)
#undef DEF_VISIT

 private:
  char* output_;  // output string buffer
  int size_;  // output_ size
  int pos_;  // current printing position

 protected:
  void Init();
  void Print(const char* format, ...);
  const char* Output() const { return output_; }

  virtual void PrintStatements(ZoneList<Statement*>* statements);
  void PrintLabels(ZoneStringList* labels);
  virtual void PrintArguments(ZoneList<Expression*>* arguments);
  void PrintLiteral(Handle<Object> value, bool quote);
  void PrintParameters(Scope* scope);
  void PrintDeclarations(ZoneList<Declaration*>* declarations);
  void PrintFunctionLiteral(FunctionLiteral* function);
  void PrintCaseClause(CaseClause* clause);
};


// Prints the AST structure
class AstPrinter: public PrettyPrinter {
 public:
  AstPrinter();
  virtual ~AstPrinter();

  const char* PrintProgram(FunctionLiteral* program);

  // Individual nodes
#define DEF_VISIT(type)                         \
  virtual void Visit##type(type* node);
  NODE_LIST(DEF_VISIT)
#undef DEF_VISIT
 private:
  friend class IndentedScope;
  void PrintIndented(const char* txt);
  void PrintIndentedVisit(const char* s, Node* node);

  void PrintStatements(ZoneList<Statement*>* statements);
  void PrintDeclarations(ZoneList<Declaration*>* declarations);
  void PrintParameters(Scope* scope);
  void PrintArguments(ZoneList<Expression*>* arguments);
  void PrintCaseClause(CaseClause* clause);
  void PrintLiteralIndented(const char* info, Handle<Object> value, bool quote);
  void PrintLiteralWithModeIndented(const char* info,
                                    Variable* var,
                                    Handle<Object> value);
  void PrintLabelsIndented(const char* info, ZoneStringList* labels);

  void inc_indent() { indent_++; }
  void dec_indent() { indent_--; }

  static int indent_;
};

#endif  // DEBUG

} }  // namespace v8::internal

#endif  // V8_PRETTYPRINTER_H_
