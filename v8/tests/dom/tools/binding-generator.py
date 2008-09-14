from idlparser import IDLParser, OutputStream
from optparse import OptionParser

def to_lower_name(name):
  result = ''
  last_was_upper = True
  for c in name:
    if c.isupper():
      if not last_was_upper: result += '_'
      result += c.lower()
      last_was_upper = True
    else:
      last_was_upper = False
      result += c
  return result

H_HEADER_TEMPLATE = """\
#include <v8.h>
#include "%(header)s"
@!
"""

DECLARATION_TEMPLATE = """
class %(name)sGenerator {
public:
  static v8::Persistent<v8::FunctionTemplate> GetTemplate();
private:
  static v8::Persistent<v8::FunctionTemplate> cached_%(lower_name)s_template_;
};
@!"""

CPP_HEADER_TEMPLATE = """\
#include "src/binding-utils.h"
#include "%(header)s"
"""

IMPLEMENTATION_TEMPLATE = """
@<\
v8::Persistent<v8::FunctionTemplate> %(name)sGenerator::GetTemplate() {
  if (!cached_%(lower_name)s_template_.IsEmpty())
    return cached_%(lower_name)s_template_;
  v8::HandleScope scope;
  v8::Persistent<v8::FunctionTemplate> templ = CreateFunctionTemplate();
  cached_%(lower_name)s_template_ = templ;
  templ->SetInternalFieldCount(2);
  v8::Local<v8::Signature> signature = v8::Signature::New(templ);
@+@<@-\
  return templ;
}
"""

METHOD_TEMPLATE = """\
templ->PrototypeTemplate()->Set(v8::String::New("%(name)s"), v8::FunctionTemplate::New(%(interface)s_%(name)s, v8::Handle<v8::Value>(), signature));
"""

ATTRIBUTE_TEMPLATE = """\
templ->PrototypeTemplate()->SetAccessor(v8::String::New("%(name)s"), Get_%(interface)s_%(name)s, %(setter)s);
"""

CONSTANT_TEMPLATE = """\
templ->Set(v8::String::New("%(name)s"), v8::Integer::New(%(value)s));
"""

GETTER_TEMPLATE = """\
static v8::Handle<v8::Value> Get_%(interface)s_%(name)s(v8::Handle<v8::String>, v8::AccessorInfo& info) {
  v8::Handle<v8::Object> holder = v8::Handle<v8::Object>::Cast(info.Holder());
  v8::Handle<v8::External> obj = v8::Handle<v8::External>::Cast(holder->GetInternal(0));
  %(interface)s* node = static_cast<%(interface)s*>(obj->Value());
  node->%(name)s();
}

"""

SETTER_TEMPLATE = """\
static void Set_%(interface)s_%(name)s(v8::Handle<v8::String>, v8::Handle<v8::Value> value, v8::AccessorInfo& info) {
  v8::Handle<v8::Object> holder = v8::Handle<v8::Object>::Cast(info.Holder());
  v8::Handle<v8::External> obj = v8::Handle<v8::External>::Cast(holder->GetInternal(0));
  %(interface)s* node = static_cast<%(interface)s*>(obj->Value());
}

"""

METHOD_IMPL_TEMPLATE = """\
static v8::Handle<v8::Value> %(interface)s_%(name)s(v8::Arguments& args) {

}

"""

class HeaderGenerator:

  def __init__(self, out, header):
    self.out = out
    self.header = header

  def visit_module(self, module):
    self.out.format(H_HEADER_TEMPLATE) % {
      'header': self.header
    }
    module.traverse(self)

  def visit_interface(self, interface):
    self.out.format(DECLARATION_TEMPLATE) % {
      'name': interface.name,
      'lower_name': to_lower_name(interface.name)
    }


class ImplementationGenerator:

  def __init__(self, out):
    self.out = out
    self.config = None
    self.impls = None
    self.current_interface = None

  def visit_module(self, module):
    module.traverse(self)

  def visit_interface(self, interface):
    self.current_interface = interface
    [self.impls, self.config] = self.out.format(IMPLEMENTATION_TEMPLATE) % {
      'name': interface.name,
      'lower_name': to_lower_name(interface.name)
    }
    interface.traverse(self)

  def visit_operation(self, operation):
    self.config.format(METHOD_TEMPLATE) % {
      'name': operation.name,
      'interface': self.current_interface.name
    }
    self.impls.format(METHOD_IMPL_TEMPLATE) % {
      'name': operation.name,
      'interface': self.current_interface.name
    }

  def visit_attribute(self, attrib):
    mapping = {
      'name': attrib.name,
      'interface': self.current_interface.name
    }
    if not attrib.mode == IDLParser.READONLY:
      self.impls.format(SETTER_TEMPLATE) % mapping
      mapping['setter'] = 'Set_%(interface)s_%(name)s' % mapping
    else:
      mapping['setter'] = 'NULL'
    self.config.format(ATTRIBUTE_TEMPLATE) % mapping
    self.impls.format(GETTER_TEMPLATE) % mapping

  def visit_constant(self, const):
    self.config.format(CONSTANT_TEMPLATE) % {
      'name': const.name,
      'value': const.value
    }


def generate_header(module, dom_name):
  out = OutputStream()
  module.accept(HeaderGenerator(out, dom_name))
  return out.flush()

def generate_implementation(module, header_name):
  out = OutputStream()
  out.format(CPP_HEADER_TEMPLATE) % {
    'header': header_name
  }
  module.accept(ImplementationGenerator(out))
  return out.flush()

# Entry-point method
def main():
  opt_parser = OptionParser()
  opt_parser.add_option("-o", dest="out", nargs=2)
  opt_parser.add_option("-d", dest="header")
  (options, args) = opt_parser.parse_args()
  parser = IDLParser()
  for name in args:
    text = open(name).read()
    parser.parse(name, text + '$')
  module = parser.module()
  header_name = options.out[0]
  dom_name = options.header
  header = open(header_name, "w")
  header.write(generate_header(module, dom_name))
  header.close()
  impl_name = options.out[1]
  impl = open(impl_name, "w")
  impl.write(generate_implementation(module, header_name))
  impl.close()

if __name__ == "__main__":
  main()
