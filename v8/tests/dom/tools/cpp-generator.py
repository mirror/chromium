import sys
from idlparser import IDLParser, OutputStream
from optparse import OptionParser

TYPE_MAPPING = {
  IDLParser.BOOLEAN:            'bool',
  IDLParser.VOID:               'void',
  IDLParser.UNSIGNED_SHORT_INT: 'unsigned short',
  IDLParser.UNSIGNED_LONG_INT:  'unsigned long',
  IDLParser.SIGNED_LONG_INT:    'long'
}

def type_to_string(idl_type):
  if idl_type in TYPE_MAPPING:
    return TYPE_MAPPING[idl_type]
  else:
    return idl_type + '*'

def format_params(params):
  strs = []
  for param in params:
    param_str = '%(type)s %(name)s' % {
     'type': type_to_string(param.type),
      'name': param.name
    }
    strs.append(param_str)
  return ', '.join(strs)

CLASS_DECL_TEMPLATE = """\
class %(name)s%(supers)s {
public:
  %(name)s();
  virtual ~%(name)s();@+
@<\
@-private:@+
@<\
@-};
@!
"""

class HeaderGenerator:

  def __init__(self, forwards, decls):
    self.forwards = forwards
    self.decls = decls
    self.public = None
    self.private = None

  def visit_module(self, module):
    module.traverse(self)

  def visit_interface(self, interface):
    # Forward declaration
    self.forwards.format('class %(name)s;\n') % {
      'name': interface.name
    }
    # Declaration
    if len(interface.supers) is 0: supers = ''
    else: supers = ': public ' + ', '.join(interface.supers)
    [self.public, self.private] = self.decls.format(CLASS_DECL_TEMPLATE) % {
      'name': interface.name,
      'supers': supers
    }
    interface.traverse(self)

  def visit_operation(self, operation):
    self.public.format('%(return_type)s %(name)s(%(params)s);\n') % {
      'return_type': type_to_string(operation.return_type),
      'name': operation.name,
      'params': format_params(operation.params)
    }

  def visit_attribute(self, attrib):
    self.private.format('%(return_type)s %(name)s_;\n') % {
      'return_type': type_to_string(attrib.type),
      'name': attrib.name
    }
    self.public.format('%(return_type)s %(name)s() const;\n') % {
      'return_type': type_to_string(attrib.type),
      'name': attrib.name
    }

  def visit_constant(self, const):
    pass

HEADER_FILE_TEMPLATE = """\
#include "src/header.h"

// --- F o r w a r d   D e c l a r a t i o n s ---

@<\

// --- D e c l a r a t i o n s ---

@<\
"""

def generate_header(module):
  out = OutputStream()
  [forwards, decls] = out.format(HEADER_FILE_TEMPLATE) % { }
  visitor = HeaderGenerator(forwards, decls)
  module.accept(visitor)
  return out.flush()

class ImplementationGenerator:

  def __init__(self, out):
    self.out = out
    self.current_interface = None

  def visit_module(self, module):
    module.traverse(self)

  def visit_interface(self, interface):
    self.current_interface = interface
    interface.traverse(self)
    self.current_interface = None

  def visit_operation(self, operation):
    self.out.format('%(return_type)s %(interface)s::%(name)s(%(params)s) {\n}\n@!\n') % {
      'return_type': type_to_string(operation.return_type),
      'interface': self.current_interface.name,
      'name': operation.name,
      'params': format_params(operation.params)
    }

  def visit_attribute(self, attrib):
    self.out.format('%(type)s %(interface)s::%(name)s() const {\n}\n@!\n') % {
      'type': type_to_string(attrib.type),
      'interface': self.current_interface.name,
      'name': attrib.name
    }

  def visit_constant(self, const):
    pass

IMPLEMENTATION_FILE_TEMPLATE = """\
#include "%(header)s"

"""

def generate_implementation(header, module):
  out = OutputStream()
  out.format(IMPLEMENTATION_FILE_TEMPLATE) % {
    'header': header
  }
  visitor = ImplementationGenerator(out)
  module.accept(visitor)
  return out.flush()

# Entry-point method
def main():
  opt_parser = OptionParser()
  opt_parser.add_option("-o", dest="out", nargs=2)
  (options, args) = opt_parser.parse_args()
  parser = IDLParser()
  for name in args:
    text = open(name).read()
    parser.parse(name, text + '$')
  module = parser.module()
  header_name = options.out[0]
  header = open(header_name, "w")
  header.write(generate_header(module))
  header.close()
  impl_name = options.out[1]
  impl = open(impl_name, "w")
  impl.write(generate_implementation(header_name, module))
  impl.close()

if __name__ == "__main__":
  main()
