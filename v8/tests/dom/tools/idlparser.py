import idl, types

# Syntax tree nodes.  Syntax tree nodes are "dumb" data structures
# with a set of properties.  Use visitors to inspect the structure of
# a syntax tree.

class SyntaxTree:
  def __str__(self):
    visitor = PrintVisitor()
    self.accept(visitor)
    return visitor.output()

class Module(SyntaxTree):

  def __init__(self):
    self.interfaces = { }

  def add_interface(self, interface):
    self.interfaces[interface.name] = interface

  def accept(self, visitor):
    visitor.visit_module(self)

  def traverse(self, visitor):
    # It is convenient for some operations that super-interfaces are
    # visited before sub-interfaces so we make sure to do that
    visited = { }
    keys = self.interfaces.keys()
    keys.sort()
    while len(keys) > 0:
      key = keys[0]
      interface = self.interfaces[key]
      keys.remove(key)
      is_ready = True
      for sup in interface.supers:
        if not sup in visited:
          is_ready = False
      if not is_ready:
        keys.append(key)
        continue
      visited[key] = True
      interface.accept(visitor)

class Interface(SyntaxTree):
  def __init__(self, name, supers):
    self.name = name
    self.supers = supers
    self.members = []
  def add_member(self, member):
    self.members.append(member)
  def accept(self, visitor):
    visitor.visit_interface(self)
  def traverse(self, visitor):
    for member in self.members:
      member.accept(visitor)

class MemberDeclaration(SyntaxTree):
  pass

class Operation(MemberDeclaration):
  def __init__(self, name, return_type):
    self.name = name
    self.return_type = return_type
  def accept(self, visitor):
    visitor.visit_operation(self)
  def traverse(self, visitor):
    pass

class Attribute(MemberDeclaration):
  def __init__(self, mode, type, name):
    self.mode = mode
    self.type = type
    self.name = name
  def accept(self, visitor):
    visitor.visit_attribute(self)
  def traverse(self, visitor):
    pass

class Constant(MemberDeclaration):
  def __init__(self, type, name, value):
    self.type = type
    self.name = name
    self.value = value
  def accept(self, visitor):
    visitor.visit_constant(self)
  def traverse(self, visitor):
    pass

class Parameter(SyntaxTree):
  def __init__(self, mode, name, type):
    self.mode = mode
    self.name = name
    self.type = type
  def accept(self, visitor):
    visitor.visit_parameter(self)
  def traverse(self, visitor):
    pass

# Output stream.  An output stream can be used more or less as an
# ordinary string stream but has a few neat extensions.  Notably, it
# does not write its output immediately but just caches it.  Because
# of this, it allows you to access "sub-streams" at a particular
# position in the stream, which you can write to independently of the
# "parent" stream.  When the stream is flushed, all output written to
# the sub-stream is written at the point where the sub-stream was
# created.  Also, an output stream manages indentation.
#
# The way to write output is to use the 'format' function, like such;
#
#   [substream1, ...] = stream.format(<format string>) % {
#     key1: value1
#     key2: value2
#     ...
#   }
#
# The format string can contain references to keys which are replaced
# with the values specified in the mapping.  The format of these
# references are the same as python's string % format, that is,
# %(key)s.  Besides this, format string can also contain formatting
# directives.  @+ and @- increase and decrease indentation, and @<
# marks a position for which a sub-stream should be returned.  The
# format method returns a list that contains a substream for each
# occurrence of @< in the format string.  Here's an example:
#
#   [sub1, sub2] = stream.format("[@<][@<]") % { }
#   sub2.format("world") % { }
#   sub1.format("hello") % { }
#   print stream.flush()
#
# This example prints '[hello][world]'.

class OutputStream:

  NEWLINE = 1
  INDENT = 2
  DEINDENT = 3

  def __init__(self, indent = 0):
    self.tokens = []
    self.string_buffer = ''

  def _print_object(self, obj):
    if type(obj) == types.LambdaType:
      result = obj()
      if result: self.string_buffer += result
    else:
      self.string_buffer += str(obj)

  def format(self, format):
    return Formatter(format, self)

  def _printf(self, format, *args):
    subs = []
    i = 0
    while i < len(format):
      c = format[i]
      if c == '@':
        i += 1
        d = format[i]
        if d == '+':
          self._flush_string_buffer()
          self.tokens.append(OutputStream.INDENT)
        elif d == '-':
          self._flush_string_buffer()
          self.tokens.append(OutputStream.DEINDENT)
        elif d == '@':
          self.string_buffer.append('@')
        elif d == '<':
          subs.append(self.get_sub_stream())
        elif '0' <= d and d <= '9':
          arg = args[ord(d) - ord('0')]
          self._print_object(arg)
      elif ord(c) == ord('\n'): # WTF?!?
        self._flush_string_buffer()
        self.tokens.append(OutputStream.NEWLINE)
      else:
        self.string_buffer += c
      i += 1
    self._flush_string_buffer()
    return subs

  def get_sub_stream(self):
    result = OutputStream()
    self._flush_string_buffer()
    self.tokens.append(result)
    return result

  def flush(self):
    self._flush_string_buffer()
    state = FlushState()
    self._flush_to(state)
    state.check_pending_newline()
    return ''.join(state.out)

  def _flush_string_buffer(self):
    if not self.string_buffer == '':
      self.tokens.append(self.string_buffer)
      self.string_buffer = ''

  def _flush_to(self, state):
    for token in self.tokens:
      if token.__class__ == OutputStream:
        token._flush_to(state)
      elif type(token) == types.IntType:
        if token == OutputStream.NEWLINE:
          state.check_pending_newline()
          state.pending_newline = True
        elif token == OutputStream.INDENT:
          state.indent += 1
        elif token == OutputStream.DEINDENT:
          state.indent -= 1
      else:
        state.append(token)

class Formatter:

  def __init__(self, format, out):
    self.format = format
    self.out = out

  def __mod__(self, mapping):
    return self.out._printf(self.format % mapping)

class FlushState:

  def __init__(self):
    self.out = []
    self.indent = 0
    self.pending_newline = False

  def check_pending_newline(self):
    if self.pending_newline:
      newline = '\n'
      self.pending_newline = False
      for i in xrange(0, self.indent):
        newline += '  '
      self.out.append(newline)

  def append(self, str):
    self.check_pending_newline()
    self.out.append(str)

# Syntax tree visitor that prints the syntax tree structure to an
# output stream.

class PrintVisitor:

  def __init__(self):
    self.out = OutputStream()

  def visit_module(self, module):
    self.out.printf('module {@+\n@0\n@-}', lambda: module.traverse(self))

  def visit_interface(self, interface):
    if len(interface.supers) == 0: supers = ''
    else: supers = ': ' + ', '.join(interface.supers)
    self.out.printf('interface @0@1 {@+\n@2\n@-}\n',
      interface.name,
      supers,
      lambda: interface.traverse(self)
    )

  def visit_operation(self, operation):
    self.out.printf('@0 @1(@2);\n',
      operation.return_type,
      operation.name,
      lambda: self.flatten(operation.params, ', ')
    )

  def visit_attribute(self, attrib):
    self.out.format('%(mode)s %(type)s %(name)s;\n') % {
      'mode': attrib.mode,
      'type': attrib.type,
      'name': attrib.name
    }

  def visit_constant(self, const):
    self.out.format('%(type)s %(name)s = %(value)s;\n') % {
      'type': const.type,
      'name': const.name,
      'value': const.value
    }

  def visit_parameter(self, param):
    self.out.format('%(mode)s %(type)s %(name)s') % {
      'mode': param.mode,
      'type': param.type,
      'name': param.name
    }

  def flatten(self, objs, sep):
    first = True
    for obj in objs:
      if first: first = False
      else: self.out.printf(', ')
      obj.accept(self)

  def output(self):
    return self.out.flush()

# This class sets up the parser and handles the callbacks that
# construct the abstract syntax tree.  The parser model is a bit funky
# so we have to manually maintain a stack of syntax tree nodes and
# stitch them together as we go along.

class IDLParser:

  READONLY = "readonly"
  ONEWAY = "oneway"
  IN = "in"
  OUT = "out"
  INOUT = "inout"
  VOID = "void"
  BOOLEAN = "boolean"
  UNSIGNED_SHORT_INT = "unsigned short"
  UNSIGNED_LONG_INT = "unsigned long"
  SIGNED_LONG_INT = "long"

  def __init__(self):
    self.result = Module()
    self.stack = [ self.result ]

  def top(self):
    return self.stack[-1]

  def module(self):
    return self.top()

  def parse(self, name, text):
    tokens = idl.IDLScanner(text + '$')
    parser = idl.IDLParser(tokens)
    parser.p = self
    parser.c = IDLParser
    try:
      parser.start()
      return self.result
    except idl.SyntaxError, e:
      line = tokens.yyfileandlineno()
      if type(line) == types.TupleType:
        file, line = line
      else:
        file = name
      print "%s:%s: %s" % (file, line, e.msg)
      return None

  def list_empty(self):
    return []

  def list_insert(self, item, list):
    if type(item) == types.ListType:
      list = item + list
    else:
      list.insert(0, item)
    return list

  def interface_dcl_header(self, (modifiers, name), inheritance):
    self.stack.append(Interface(name, inheritance))

  def interface_dcl_body(self, inte):
    interface = self.stack.pop()
    self.top().add_interface(interface)

  def idl_type_scoped_name(self, name):
    return name

  def simple_declarator(self, name):
    return name

  def attr_dcl(self, mode, type, names):
    for name in names:
      self.top().add_member(Attribute(mode, type, name))

  def const_dcl(self, type, name, value):
    self.top().add_member(Constant(type, name, value))

  def op_dcl_header(self, mode, result_type, name):
    self.stack.append(Operation(name, result_type))

  def param_dcl(self, mode, type, name):
    return Parameter(mode, name, type)

  def op_dcl_body(self, operation, params, exceptions, contexts):
    op = self.stack.pop()
    op.params = params
    decl = [operation, params, exceptions, contexts]
    self.top().add_member(op)
