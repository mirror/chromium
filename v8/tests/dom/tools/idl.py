

from string import *
import re
from yappsrt import *

class IDLScanner(Scanner):
    patterns = [
        ("'>'", re.compile('>')),
        ("'<'", re.compile('<')),
        ("'~'", re.compile('~')),
        ("'%'", re.compile('%')),
        ("'/'", re.compile('/')),
        ("'-'", re.compile('-')),
        ("'&'", re.compile('&')),
        ("'='", re.compile('=')),
        ("','", re.compile(',')),
        ("':'", re.compile(':')),
        ("'}'", re.compile('}')),
        ("'{'", re.compile('{')),
        ("';'", re.compile(';')),
        ('[ \n\r\t]+', re.compile('[ \n\r\t]+')),
        ('#[ \t]*ident.*\n', re.compile('#[ \t]*ident.*\n')),
        ('//.*\n', re.compile('//.*\n')),
        ('PRAGMA', re.compile('#[ \t]*pragma.*\n|#[\\ \t]*[0-9]+[ \t]*pragma.*\n')),
        ('LINE', re.compile('#[ \t]*[0-9].*\n|#[ \t]*line.*\n')),
        ('ABSTRACT', re.compile('abstract')),
        ('IDL_ANY', re.compile('any')),
        ('ATTRIBUTE', re.compile('attribute')),
        ('BOOLEAN', re.compile('boolean')),
        ('CASE', re.compile('case')),
        ('CHAR', re.compile('char')),
        ('CONST', re.compile('const')),
        ('CONTEXT', re.compile('context')),
        ('CUSTOM', re.compile('custom')),
        ('DEFAULT', re.compile('default')),
        ('DOUBLE', re.compile('double')),
        ('ENUM', re.compile('enum')),
        ('EXCEPTION', re.compile('exception')),
        ('FACTORY', re.compile('factory')),
        ('IDL_FALSE', re.compile('FALSE')),
        ('FIXED', re.compile('fixed')),
        ('FLOAT', re.compile('float')),
        ('IN', re.compile('in')),
        ('INOUT', re.compile('inout')),
        ('INTERFACE', re.compile('interface')),
        ('LOCAL', re.compile('local')),
        ('LONG', re.compile('long')),
        ('MODULE', re.compile('module')),
        ('NATIVE', re.compile('native')),
        ('OBJECT', re.compile('Object')),
        ('OCTET', re.compile('octet')),
        ('ONEWAY', re.compile('oneway')),
        ('OUT', re.compile('out')),
        ('PRIVATE', re.compile('private')),
        ('PUBLIC', re.compile('public')),
        ('RAISES', re.compile('raises')),
        ('READONLY', re.compile('readonly')),
        ('SCOPE_DELIMITER', re.compile('::')),
        ('SEQUENCE', re.compile('sequence')),
        ('SHORT', re.compile('short')),
        ('STRING', re.compile('string')),
        ('STRUCT', re.compile('struct')),
        ('SUPPORTS', re.compile('supports')),
        ('SWITCH', re.compile('switch')),
        ('IDL_TRUE', re.compile('TRUE')),
        ('TRUNCATABLE', re.compile('truncatable')),
        ('TYPEDEF', re.compile('typedef')),
        ('UNION', re.compile('union')),
        ('UNSIGNED', re.compile('unsigned')),
        ('VALUEBASE', re.compile('ValueBase')),
        ('VALUETYPE', re.compile('valuetype')),
        ('VOID', re.compile('void')),
        ('WCHAR', re.compile('wchar')),
        ('WSTRING', re.compile('wstring')),
        ('IDENTIFIER', re.compile('[_a-zA-Z][a-zA-Z0-9_]*')),
        ('CHARACTER_LITERAL', re.compile("'.'")),
        ('WIDE_CHARACTER_LITERAL', re.compile("L'.'")),
        ('FLOATING_PT_LITERAL', re.compile('([0-9]+\\.[0-9]*|\\.[0-9]+)([eE][+-]?[0-9]+)?|[0-9]+[eE][+-]?[0-9]+')),
        ('INTEGER_LITERAL', re.compile('(0|0x)?[0-9a-fA-F]+')),
        ('STRING_LITERAL', re.compile('"([^\\\\"]+|\\\\.)*"')),
        ('WIDE_STRING_LITERAL', re.compile('L"([^\\\\"]+|\\\\.)*"')),
        ('FIXED_PT_LITERAL', re.compile('[0-9]+\\.[0-9]*D')),
        ('LEFT_SHIFT', re.compile('<<')),
        ('RIGHT_SHIFT', re.compile('>>')),
        ('LBRACKET', re.compile('\\[')),
        ('RBRACKET', re.compile('\\]')),
        ('LPAREN', re.compile('\\(')),
        ('RPAREN', re.compile('\\)')),
        ('PLUS', re.compile('\\+')),
        ('STAR', re.compile('\\*')),
        ('BAR', re.compile('\\|')),
        ('CARET', re.compile('\\^')),
        ('END', re.compile('\\$')),
    ]
    def __init__(self, str):
        Scanner.__init__(self,None,['[ \n\r\t]+', '#[ \t]*ident.*\n', '//.*\n'],str)

class IDL(Parser):
    def start(self):
        _token_ = self._peek()
        if _token_ == 'END':
            END = self._scan('END')
        elif _token_ in ['ABSTRACT', 'PRAGMA', 'TYPEDEF', 'NATIVE', 'CONST', 'EXCEPTION', 'INTERFACE', 'MODULE', 'STRUCT', 'UNION', 'ENUM', 'LOCAL', 'VALUETYPE', 'CUSTOM']:
            specification = self.specification()
            END = self._scan('END')
        else:
            raise SyntaxError(self._pos, 'Could not match start')

    def specification(self):
        
        while 1:
            definition = self.definition()
            if self._peek() not in ['ABSTRACT', 'PRAGMA', 'TYPEDEF', 'NATIVE', 'CONST', 'EXCEPTION', 'INTERFACE', 'MODULE', 'STRUCT', 'UNION', 'ENUM', 'LOCAL', 'VALUETYPE', 'CUSTOM']: break
        

    def definition(self):
        _token_ = self._peek()
        if _token_ in ['TYPEDEF', 'NATIVE', 'STRUCT', 'UNION', 'ENUM']:
            type_dcl = self.type_dcl()
            self._scan("';'")
        elif _token_ == 'CONST':
            const_dcl = self.const_dcl()
            self._scan("';'")
        elif _token_ == 'EXCEPTION':
            except_dcl = self.except_dcl()
            self._scan("';'")
        elif _token_ in ['INTERFACE', 'LOCAL']:
            interface = self.interface()
            self._scan("';'")
        elif _token_ == 'MODULE':
            module = self.module()
            self._scan("';'")
        elif _token_ in ['VALUETYPE', 'CUSTOM']:
            value = self.value()
            self._scan("';'")
        elif _token_ == 'ABSTRACT':
            ABSTRACT = self._scan('ABSTRACT')
            abstract_thing = self.abstract_thing()
            self._scan("';'")
        elif _token_ == 'PRAGMA':
            PRAGMA = self._scan('PRAGMA')
            self.p.pragma(PRAGMA)
        else:
            raise SyntaxError(self._pos, 'Could not match definition')

    def module(self):
        MODULE = self._scan('MODULE')
        IDENTIFIER = self._scan('IDENTIFIER')
        self.p.module_header(IDENTIFIER)
        self._scan("'{'")
        while 1:
            definition = self.definition()
            if self._peek() not in ['ABSTRACT', 'PRAGMA', 'TYPEDEF', 'NATIVE', 'CONST', 'EXCEPTION', 'INTERFACE', 'MODULE', 'STRUCT', 'UNION', 'ENUM', 'LOCAL', 'VALUETYPE', 'CUSTOM']: break
        self._scan("'}'")
        self.p.module_body()

    def abstract_thing(self):
        _token_ = self._peek()
        if _token_ == 'INTERFACE':
            INTERFACE = self._scan('INTERFACE')
            IDENTIFIER = self._scan('IDENTIFIER')
            interface_body_opt = self.interface_body_opt(("abstract", IDENTIFIER))
            raise NotImplementedError
        elif _token_ == 'VALUETYPE':
            VALUETYPE = self._scan('VALUETYPE')
            IDENTIFIER = self._scan('IDENTIFIER')
            value_inheritance_spec = self.value_inheritance_spec()
            res = self.p.value_dcl_header(("abstract", IDENTIFIER), 
                                          value_inheritance_spec)
            self._scan("'{'")
            while self._peek() in ['PRAGMA', 'TYPEDEF', 'NATIVE', 'CONST', 'EXCEPTION', 'ATTRIBUTE', 'STRUCT', 'UNION', 'ENUM', 'READONLY', 'ONEWAY', 'VOID', 'STRING', 'WSTRING', 'IDENTIFIER', 'SCOPE_DELIMITER', 'FLOAT', 'DOUBLE', 'SHORT', 'LONG', 'UNSIGNED', 'CHAR', 'WCHAR', 'BOOLEAN', 'OCTET', 'IDL_ANY', 'OBJECT', 'FIXED']:
                export = self.export()
            self._scan("'}'")
            self.p.value_dcl_body(res)
        else:
            raise SyntaxError(self._pos, 'Could not match abstract_thing')

    def interface(self):
        local_opt = self.local_opt()
        INTERFACE = self._scan('INTERFACE')
        IDENTIFIER = self._scan('IDENTIFIER')
        interface_body_opt = self.interface_body_opt((local_opt, IDENTIFIER))

    def local_opt(self):
        _token_ = self._peek()
        if _token_ == 'INTERFACE':
            return None
        elif _token_ == 'LOCAL':
            LOCAL = self._scan('LOCAL')
            return "local"
        else:
            raise SyntaxError(self._pos, 'Could not match local_opt')

    def interface_body_opt(self, ident):
        _token_ = self._peek()
        if _token_ == "';'":
            self.p.foward_dcl(ident)
        elif _token_ in ["'{'", "':'"]:
            interface_inheritance_spec = self.interface_inheritance_spec()
            res = self.p.interface_dcl_header(ident, interface_inheritance_spec)
            self._scan("'{'")
            while self._peek() in ['PRAGMA', 'TYPEDEF', 'NATIVE', 'CONST', 'EXCEPTION', 'ATTRIBUTE', 'STRUCT', 'UNION', 'ENUM', 'READONLY', 'ONEWAY', 'VOID', 'STRING', 'WSTRING', 'IDENTIFIER', 'SCOPE_DELIMITER', 'FLOAT', 'DOUBLE', 'SHORT', 'LONG', 'UNSIGNED', 'CHAR', 'WCHAR', 'BOOLEAN', 'OCTET', 'IDL_ANY', 'OBJECT', 'FIXED']:
                export = self.export()
            self._scan("'}'")
            self.p.interface_dcl_body(res)
        else:
            raise SyntaxError(self._pos, 'Could not match interface_body_opt')

    def export(self):
        _token_ = self._peek()
        if _token_ in ['TYPEDEF', 'NATIVE', 'STRUCT', 'UNION', 'ENUM']:
            type_dcl = self.type_dcl()
            self._scan("';'")
        elif _token_ == 'CONST':
            const_dcl = self.const_dcl()
            self._scan("';'")
        elif _token_ == 'EXCEPTION':
            except_dcl = self.except_dcl()
            self._scan("';'")
        elif _token_ in ['ATTRIBUTE', 'READONLY']:
            attr_dcl = self.attr_dcl()
            self._scan("';'")
        elif _token_ in ['ONEWAY', 'VOID', 'STRING', 'WSTRING', 'IDENTIFIER', 'SCOPE_DELIMITER', 'FLOAT', 'DOUBLE', 'SHORT', 'LONG', 'UNSIGNED', 'CHAR', 'WCHAR', 'BOOLEAN', 'OCTET', 'IDL_ANY', 'OBJECT', 'FIXED']:
            op_dcl = self.op_dcl()
            self._scan("';'")
        elif _token_ == 'PRAGMA':
            PRAGMA = self._scan('PRAGMA')
            self.p.pragma(PRAGMA)
        else:
            raise SyntaxError(self._pos, 'Could not match export')

    def interface_inheritance_spec(self):
        _token_ = self._peek()
        if _token_ == "'{'":
            return []
        elif _token_ == "':'":
            self._scan("':'")
            scoped_name_CSV_PLUS = self.scoped_name_CSV_PLUS()
            return scoped_name_CSV_PLUS
        else:
            raise SyntaxError(self._pos, 'Could not match interface_inheritance_spec')

    def scoped_name_CSV_PLUS(self):
        scoped_name = self.scoped_name()
        scoped_name_CSV = self.scoped_name_CSV()
        return self.p.list_insert(scoped_name, scoped_name_CSV)

    def scoped_name_CSV(self):
        _token_ = self._peek()
        if _token_ in ['RPAREN', "'{'", 'SUPPORTS']:
            return self.p.list_empty()
        elif _token_ == "','":
            self._scan("','")
            scoped_name = self.scoped_name()
            scoped_name_CSV = self.scoped_name_CSV()
            return self.p.list_insert(scoped_name, scoped_name_CSV)
        else:
            raise SyntaxError(self._pos, 'Could not match scoped_name_CSV')

    def scoped_name_0(self):
        _token_ = self._peek()
        if _token_ == 'IDENTIFIER':
            IDENTIFIER = self._scan('IDENTIFIER')
            return IDENTIFIER
        elif _token_ == 'SCOPE_DELIMITER':
            SCOPE_DELIMITER = self._scan('SCOPE_DELIMITER')
            IDENTIFIER = self._scan('IDENTIFIER')
            return self.p.scoped_name_absolute(IDENTIFIER)
        else:
            raise SyntaxError(self._pos, 'Could not match scoped_name_0')

    def scoped_name(self):
        scoped_name_0 = self.scoped_name_0()
        res = scoped_name_0
        while self._peek() == 'SCOPE_DELIMITER':
            SCOPE_DELIMITER = self._scan('SCOPE_DELIMITER')
            IDENTIFIER = self._scan('IDENTIFIER')
            res = self.p.scoped_name_relative(res, IDENTIFIER)
        return res

    def value(self):
        _token_ = self._peek()
        if _token_ == 'VALUETYPE':
            value_dcl = self.value_dcl()
        elif _token_ == 'CUSTOM':
            custom_value_dcl = self.custom_value_dcl()
        else:
            raise SyntaxError(self._pos, 'Could not match value')

    def value_dcl(self):
        VALUETYPE = self._scan('VALUETYPE')
        IDENTIFIER = self._scan('IDENTIFIER')
        value_body = self.value_body((None, IDENTIFIER))

    def custom_value_dcl(self):
        CUSTOM = self._scan('CUSTOM')
        VALUETYPE = self._scan('VALUETYPE')
        IDENTIFIER = self._scan('IDENTIFIER')
        value_body = self.value_body(("custom",IDENTIFIER))

    def value_body(self, ident):
        _token_ = self._peek()
        if _token_ == "';'":
            return self.p.value_forward_dcl(ident)
        elif _token_ in ['IDENTIFIER', 'SCOPE_DELIMITER', 'STRUCT', 'UNION', 'ENUM', 'FLOAT', 'DOUBLE', 'SHORT', 'LONG', 'UNSIGNED', 'CHAR', 'WCHAR', 'BOOLEAN', 'OCTET', 'IDL_ANY', 'OBJECT', 'SEQUENCE', 'STRING', 'WSTRING', 'FIXED']:
            type_spec = self.type_spec()
            return self.p.value_box_dcl(ident,type_spec)
        elif _token_ in ["'{'", "':'", 'SUPPORTS']:
            value_inheritance_spec = self.value_inheritance_spec()
            res = self.p.value_dcl_header(ident, 
                                          value_inheritance_spec)
            self._scan("'{'")
            while self._peek() in ['PRAGMA', 'TYPEDEF', 'NATIVE', 'CONST', 'EXCEPTION', 'ATTRIBUTE', 'FACTORY', 'STRUCT', 'UNION', 'ENUM', 'READONLY', 'ONEWAY', 'VOID', 'PUBLIC', 'PRIVATE', 'STRING', 'WSTRING', 'IDENTIFIER', 'SCOPE_DELIMITER', 'FLOAT', 'DOUBLE', 'SHORT', 'LONG', 'UNSIGNED', 'CHAR', 'WCHAR', 'BOOLEAN', 'OCTET', 'IDL_ANY', 'OBJECT', 'FIXED']:
                value_element = self.value_element(res)
            self._scan("'}'")
            self.p.value_dcl_body(res)
        else:
            raise SyntaxError(self._pos, 'Could not match value_body')

    def value_inheritance_spec(self):
        value_inherits_opt = self.value_inherits_opt()
        value_supports_opt = self.value_supports_opt()
        return (value_inherits_opt, value_supports_opt)

    def value_inherits_opt(self):
        _token_ = self._peek()
        if _token_ in ['SUPPORTS', "'{'"]:
            return None
        elif _token_ == "':'":
            self._scan("':'")
            truncatable_opt = self.truncatable_opt()
            scoped_name_CSV_PLUS = self.scoped_name_CSV_PLUS()
            return (truncatable_opt, scoped_name_CSV_PLUS)
        else:
            raise SyntaxError(self._pos, 'Could not match value_inherits_opt')

    def truncatable_opt(self):
        _token_ = self._peek()
        if _token_ in ['IDENTIFIER', 'SCOPE_DELIMITER']:
            return None
        elif _token_ == 'TRUNCATABLE':
            TRUNCATABLE = self._scan('TRUNCATABLE')
            return "truncatable"
        else:
            raise SyntaxError(self._pos, 'Could not match truncatable_opt')

    def value_supports_opt(self):
        _token_ = self._peek()
        if _token_ == "'{'":
            return None
        elif _token_ == 'SUPPORTS':
            SUPPORTS = self._scan('SUPPORTS')
            scoped_name_CSV_PLUS = self.scoped_name_CSV_PLUS()
            return scoped_name_CSV_PLUS
        else:
            raise SyntaxError(self._pos, 'Could not match value_supports_opt')

    def value_element(self, value):
        _token_ = self._peek()
        if _token_ not in ["'>'", "'<'", "'~'", "'%'", "'/'", "'-'", "'&'", "'='", "','", "':'", "'}'", "'{'", "';'", 'LINE', 'ABSTRACT', 'CASE', 'CONTEXT', 'CUSTOM', 'DEFAULT', 'FACTORY', 'IDL_FALSE', 'IN', 'INOUT', 'INTERFACE', 'LOCAL', 'MODULE', 'OUT', 'PRIVATE', 'PUBLIC', 'RAISES', 'SEQUENCE', 'SUPPORTS', 'SWITCH', 'IDL_TRUE', 'TRUNCATABLE', 'VALUEBASE', 'VALUETYPE', 'CHARACTER_LITERAL', 'WIDE_CHARACTER_LITERAL', 'FLOATING_PT_LITERAL', 'INTEGER_LITERAL', 'STRING_LITERAL', 'WIDE_STRING_LITERAL', 'FIXED_PT_LITERAL', 'LEFT_SHIFT', 'RIGHT_SHIFT', 'LBRACKET', 'RBRACKET', 'LPAREN', 'RPAREN', 'PLUS', 'STAR', 'BAR', 'CARET', 'END']:
            export = self.export()
        elif _token_ in ['PUBLIC', 'PRIVATE']:
            state_member = self.state_member()
        elif _token_ == 'FACTORY':
            init_dcl = self.init_dcl(value)
        else:
            raise SyntaxError(self._pos, 'Could not match value_element')

    def state_member(self):
        public_private = self.public_private()
        type_spec = self.type_spec()
        declarator_CSV_PLUS = self.declarator_CSV_PLUS()
        self._scan("';'")
        return self.p.state_member(public_private, 
                                                       type_spec,
                                                       declarator_CSV_PLUS)

    def public_private(self):
        _token_ = self._peek()
        if _token_ == 'PUBLIC':
            PUBLIC = self._scan('PUBLIC')
            return "public"
        elif _token_ == 'PRIVATE':
            PRIVATE = self._scan('PRIVATE')
            return "private"
        else:
            raise SyntaxError(self._pos, 'Could not match public_private')

    def init_dcl(self, value):
        FACTORY = self._scan('FACTORY')
        IDENTIFIER = self._scan('IDENTIFIER')
        parameter_dcls = self.parameter_dcls()
        self._scan("';'")
        return self.p.init_dcl(IDENTIFIER, parameter_dcls, value)

    def const_dcl(self):
        CONST = self._scan('CONST')
        const_type = self.const_type()
        IDENTIFIER = self._scan('IDENTIFIER')
        self._scan("'='")
        const_expr = self.const_expr()
        return self.p.const_dcl(const_type, IDENTIFIER, const_expr)

    def const_type(self):
        _token_ = self._peek()
        if _token_ in ['SHORT', 'LONG', 'UNSIGNED']:
            integer_type = self.integer_type()
            return integer_type
        elif _token_ == 'CHAR':
            char_type = self.char_type()
            return char_type
        elif _token_ == 'WCHAR':
            wide_char_type = self.wide_char_type()
            return wide_char_type
        elif _token_ == 'BOOLEAN':
            boolean_type = self.boolean_type()
            return boolean_type
        elif _token_ in ['FLOAT', 'DOUBLE']:
            floating_pt_type = self.floating_pt_type()
            return floating_pt_type
        elif _token_ == 'STRING':
            string_type = self.string_type()
            return string_type
        elif _token_ == 'WSTRING':
            wide_string_type = self.wide_string_type()
            return wide_string_type
        elif _token_ == 'FIXED':
            fixed_pt_const_type = self.fixed_pt_const_type()
            return fixed_pt_const_type
        elif _token_ in ['IDENTIFIER', 'SCOPE_DELIMITER']:
            scoped_name = self.scoped_name()
            return self.p.const_type_scoped_name(scoped_name)
        else:
            raise SyntaxError(self._pos, 'Could not match const_type')

    def const_expr(self):
        or_expr = self.or_expr()
        return or_expr

    def or_expr(self):
        xor_expr = self.xor_expr()
        res = xor_expr
        while self._peek() == 'BAR':
            BAR = self._scan('BAR')
            xor_expr = self.xor_expr()
            res = self.p.or_expr(res, xor_expr)
        return res

    def xor_expr(self):
        and_expr = self.and_expr()
        res = and_expr
        while self._peek() == 'CARET':
            CARET = self._scan('CARET')
            and_expr = self.and_expr()
            res = self.p.xor_expr(res, and_expr)
        return res

    def and_expr(self):
        shift_expr = self.shift_expr()
        res = shift_expr
        while self._peek() == "'&'":
            self._scan("'&'")
            shift_expr = self.shift_expr()
            res = self.p.and_expr(res, shift_expr)
        return res

    def shift_expr(self):
        add_expr = self.add_expr()
        res = add_expr
        while self._peek() in ['RIGHT_SHIFT', 'LEFT_SHIFT']:
            _token_ = self._peek()
            if _token_ == 'RIGHT_SHIFT':
                RIGHT_SHIFT = self._scan('RIGHT_SHIFT')
                add_expr = self.add_expr()
                res = self.p.shift_expr_right(res, add_expr)
            elif _token_ == 'LEFT_SHIFT':
                LEFT_SHIFT = self._scan('LEFT_SHIFT')
                add_expr = self.add_expr()
                res = self.p.shift_expr_left(res, add_expr)
            else:
                raise SyntaxError(self._pos, 'Could not match shift_expr')
        return res

    def add_expr(self):
        mult_expr = self.mult_expr()
        res = mult_expr
        while self._peek() in ['PLUS', "'-'"]:
            _token_ = self._peek()
            if _token_ == 'PLUS':
                PLUS = self._scan('PLUS')
                mult_expr = self.mult_expr()
                res = self.p.add_expr_add(res, mult_expr)
            elif _token_ == "'-'":
                self._scan("'-'")
                mult_expr = self.mult_expr()
                res = self.p.add_expr_subtract(res, mult_expr)
            else:
                raise SyntaxError(self._pos, 'Could not match add_expr')
        return res

    def mult_expr(self):
        unary_expr = self.unary_expr()
        res = unary_expr
        while self._peek() in ['STAR', "'/'", "'%'"]:
            _token_ = self._peek()
            if _token_ == 'STAR':
                STAR = self._scan('STAR')
                unary_expr = self.unary_expr()
                res = self.p.mult_expr_multiply(res, unary_expr)
            elif _token_ == "'/'":
                self._scan("'/'")
                unary_expr = self.unary_expr()
                res = self.p.mult_expr_divide(res, unary_expr)
            elif _token_ == "'%'":
                self._scan("'%'")
                unary_expr = self.unary_expr()
                res = self.p.mult_expr_mod(res, unary_expr)
            else:
                raise SyntaxError(self._pos, 'Could not match mult_expr')
        return res

    def unary_expr(self):
        _token_ = self._peek()
        if _token_ == "'-'":
            self._scan("'-'")
            primary_expr = self.primary_expr()
            return self.p.unary_expr_neg(primary_expr)
        elif _token_ == 'PLUS':
            PLUS = self._scan('PLUS')
            primary_expr = self.primary_expr()
            return self.p.unary_expr_pos(primary_expr)
        elif _token_ == "'~'":
            self._scan("'~'")
            primary_expr = self.primary_expr()
            return self.p.unary_expr_invert(primary_expr)
        elif _token_ in ['IDENTIFIER', 'SCOPE_DELIMITER', 'LPAREN', 'STRING_LITERAL', 'CHARACTER_LITERAL', 'FIXED_PT_LITERAL', 'FLOATING_PT_LITERAL', 'INTEGER_LITERAL', 'IDL_TRUE', 'IDL_FALSE']:
            primary_expr = self.primary_expr()
            return primary_expr
        else:
            raise SyntaxError(self._pos, 'Could not match unary_expr')

    def primary_expr(self):
        _token_ = self._peek()
        if _token_ in ['IDENTIFIER', 'SCOPE_DELIMITER']:
            scoped_name = self.scoped_name()
            return self.p.primary_expr_scoped_name(scoped_name)
        elif _token_ in ['STRING_LITERAL', 'CHARACTER_LITERAL', 'FIXED_PT_LITERAL', 'FLOATING_PT_LITERAL', 'INTEGER_LITERAL', 'IDL_TRUE', 'IDL_FALSE']:
            literal = self.literal()
            return literal
        elif _token_ == 'LPAREN':
            LPAREN = self._scan('LPAREN')
            const_expr = self.const_expr()
            RPAREN = self._scan('RPAREN')
            return const_expr
        else:
            raise SyntaxError(self._pos, 'Could not match primary_expr')

    def literal(self):
        _token_ = self._peek()
        if _token_ == 'INTEGER_LITERAL':
            integer_literal = self.integer_literal()
            return integer_literal
        elif _token_ == 'STRING_LITERAL':
            STRING_LITERAL = self._scan('STRING_LITERAL')
            return self.p.literal_string_literal(STRING_LITERAL)
        elif _token_ == 'CHARACTER_LITERAL':
            CHARACTER_LITERAL = self._scan('CHARACTER_LITERAL')
            return self.p.literal_character_literal(CHARACTER_LITERAL)
        elif _token_ == 'FIXED_PT_LITERAL':
            FIXED_PT_LITERAL = self._scan('FIXED_PT_LITERAL')
            return self.p.literal_fixed_pt_literal(FIXED_PT_LITERAL)
        elif _token_ == 'FLOATING_PT_LITERAL':
            FLOATING_PT_LITERAL = self._scan('FLOATING_PT_LITERAL')
            return self.p.literal_floating_pt_literal(float(FLOATING_PT_LITERAL))
        elif _token_ in ['IDL_TRUE', 'IDL_FALSE']:
            boolean_literal = self.boolean_literal()
            return boolean_literal
        else:
            raise SyntaxError(self._pos, 'Could not match literal')

    def integer_literal(self):
        INTEGER_LITERAL = self._scan('INTEGER_LITERAL')
        return INTEGER_LITERAL

    def boolean_literal(self):
        _token_ = self._peek()
        if _token_ == 'IDL_TRUE':
            IDL_TRUE = self._scan('IDL_TRUE')
            return self.p.boolean_literal_true()
        elif _token_ == 'IDL_FALSE':
            IDL_FALSE = self._scan('IDL_FALSE')
            return self.p.boolean_literal_false()
        else:
            raise SyntaxError(self._pos, 'Could not match boolean_literal')

    def positive_int_const(self):
        const_expr = self.const_expr()
        return self.p.positive_int_const(const_expr)

    def type_dcl(self):
        _token_ = self._peek()
        if _token_ == 'TYPEDEF':
            TYPEDEF = self._scan('TYPEDEF')
            type_declarator = self.type_declarator()
        elif _token_ == 'STRUCT':
            struct_type = self.struct_type()
        elif _token_ == 'UNION':
            union_type = self.union_type()
        elif _token_ == 'ENUM':
            enum_type = self.enum_type()
        elif _token_ == 'NATIVE':
            NATIVE = self._scan('NATIVE')
            simple_declarator = self.simple_declarator()
            self.p.native_type_dcl(simple_declarator)
        else:
            raise SyntaxError(self._pos, 'Could not match type_dcl')

    def type_declarator(self):
        type_spec = self.type_spec()
        declarator_CSV_PLUS = self.declarator_CSV_PLUS()
        self.p.type_declarator(type_spec, declarator_CSV_PLUS)

    def type_spec(self):
        _token_ = self._peek()
        if _token_ in ['IDENTIFIER', 'SCOPE_DELIMITER', 'FLOAT', 'DOUBLE', 'SHORT', 'LONG', 'UNSIGNED', 'CHAR', 'WCHAR', 'BOOLEAN', 'OCTET', 'IDL_ANY', 'OBJECT', 'SEQUENCE', 'STRING', 'WSTRING', 'FIXED']:
            simple_type_spec = self.simple_type_spec()
            return simple_type_spec
        elif _token_ in ['STRUCT', 'UNION', 'ENUM']:
            constr_type_spec = self.constr_type_spec()
            return constr_type_spec
        else:
            raise SyntaxError(self._pos, 'Could not match type_spec')

    def simple_type_spec(self):
        _token_ = self._peek()
        if _token_ in ['FLOAT', 'DOUBLE', 'SHORT', 'LONG', 'UNSIGNED', 'CHAR', 'WCHAR', 'BOOLEAN', 'OCTET', 'IDL_ANY', 'OBJECT']:
            base_type_spec = self.base_type_spec()
            return base_type_spec
        elif _token_ in ['SEQUENCE', 'STRING', 'WSTRING', 'FIXED']:
            template_type_spec = self.template_type_spec()
            return template_type_spec
        elif _token_ in ['IDENTIFIER', 'SCOPE_DELIMITER']:
            scoped_name = self.scoped_name()
            return self.p.idl_type_scoped_name(scoped_name)
        else:
            raise SyntaxError(self._pos, 'Could not match simple_type_spec')

    def base_type_spec(self):
        _token_ = self._peek()
        if _token_ in ['FLOAT', 'DOUBLE']:
            floating_pt_type = self.floating_pt_type()
            return floating_pt_type
        elif _token_ in ['SHORT', 'LONG', 'UNSIGNED']:
            integer_type = self.integer_type()
            return integer_type
        elif _token_ == 'CHAR':
            char_type = self.char_type()
            return char_type
        elif _token_ == 'WCHAR':
            wide_char_type = self.wide_char_type()
            return wide_char_type
        elif _token_ == 'BOOLEAN':
            boolean_type = self.boolean_type()
            return boolean_type
        elif _token_ == 'OCTET':
            octet_type = self.octet_type()
            return octet_type
        elif _token_ == 'IDL_ANY':
            any_type = self.any_type()
            return any_type
        elif _token_ == 'OBJECT':
            object_type = self.object_type()
            return object_type
        else:
            raise SyntaxError(self._pos, 'Could not match base_type_spec')

    def template_type_spec(self):
        _token_ = self._peek()
        if _token_ == 'SEQUENCE':
            sequence_type = self.sequence_type()
            return sequence_type
        elif _token_ == 'STRING':
            string_type = self.string_type()
            return string_type
        elif _token_ == 'WSTRING':
            wide_string_type = self.wide_string_type()
            return wide_string_type
        elif _token_ == 'FIXED':
            fixed_pt_type = self.fixed_pt_type()
            return fixed_pt_type
        else:
            raise SyntaxError(self._pos, 'Could not match template_type_spec')

    def constr_type_spec(self):
        _token_ = self._peek()
        if _token_ == 'STRUCT':
            struct_type = self.struct_type()
            return struct_type
        elif _token_ == 'UNION':
            union_type = self.union_type()
            return union_type
        elif _token_ == 'ENUM':
            enum_type = self.enum_type()
            return enum_type
        else:
            raise SyntaxError(self._pos, 'Could not match constr_type_spec')

    def declarator_CSV_PLUS(self):
        declarator = self.declarator()
        declarator_CSV = self.declarator_CSV()
        return self.p.list_insert(declarator, declarator_CSV)

    def declarator_CSV(self):
        res = self.p.list_empty()
        while self._peek() == "','":
            self._scan("','")
            declarator = self.declarator()
            self.p.list_insert(declarator, res)
        return res

    def declarator(self):
        IDENTIFIER = self._scan('IDENTIFIER')
        arrays = []
        while self._peek() == 'LBRACKET':
            fixed_array_size = self.fixed_array_size()
            arrays.append(fixed_array_size)
        return self.do_declarator(IDENTIFIER, arrays)

    def simple_declarator(self):
        IDENTIFIER = self._scan('IDENTIFIER')
        return self.p.simple_declarator(IDENTIFIER)

    def floating_pt_type(self):
        _token_ = self._peek()
        if _token_ == 'FLOAT':
            FLOAT = self._scan('FLOAT')
            return self.p.float_type()
        elif _token_ == 'DOUBLE':
            DOUBLE = self._scan('DOUBLE')
            return self.p.double_type()
        else:
            raise SyntaxError(self._pos, 'Could not match floating_pt_type')

    def integer_type(self):
        _token_ = self._peek()
        if _token_ == 'SHORT':
            SHORT = self._scan('SHORT')
            return self.p.signed_short_int()
        elif _token_ == 'LONG':
            LONG = self._scan('LONG')
            _token_ = self._peek()
            if _token_ in ['LONG', 'IDENTIFIER', 'RPAREN', "'>'", "','", "';'"]:
                long_long = self.long_long(0)
                return long_long
            elif _token_ == 'DOUBLE':
                DOUBLE = self._scan('DOUBLE')
                return self.p.longdouble_type()
            else:
                raise SyntaxError(self._pos, 'Could not match integer_type')
        elif _token_ == 'UNSIGNED':
            UNSIGNED = self._scan('UNSIGNED')
            _token_ = self._peek()
            if _token_ == 'SHORT':
                SHORT = self._scan('SHORT')
                return self.p.UNSIGNED_SHORT_INT
            elif _token_ == 'LONG':
                LONG = self._scan('LONG')
                long_long = self.long_long(1)
                return self.p.UNSIGNED_LONG_INT
            else:
                raise SyntaxError(self._pos, 'Could not match integer_type')
        else:
            raise SyntaxError(self._pos, 'Could not match integer_type')

    def long_long(self, unsigned):
        _token_ = self._peek()
        if _token_ in ['IDENTIFIER', 'RPAREN', "'>'", "','", "';'"]:
            return self.do_long_long(unsigned, 0)
        elif _token_ == 'LONG':
            LONG = self._scan('LONG')
            return self.do_long_long(unsigned, 1)
        else:
            raise SyntaxError(self._pos, 'Could not match long_long')

    def char_type(self):
        CHAR = self._scan('CHAR')
        return self.p.char_type()

    def wide_char_type(self):
        WCHAR = self._scan('WCHAR')
        return self.p.wide_char_type()

    def boolean_type(self):
        BOOLEAN = self._scan('BOOLEAN')
        return self.p.BOOLEAN

    def octet_type(self):
        OCTET = self._scan('OCTET')
        return self.p.octet_type()

    def any_type(self):
        IDL_ANY = self._scan('IDL_ANY')
        return self.p.any_type()

    def object_type(self):
        OBJECT = self._scan('OBJECT')
        return self.p.object_type()

    def struct_type(self):
        STRUCT = self._scan('STRUCT')
        IDENTIFIER = self._scan('IDENTIFIER')
        res = self.p.struct_type_header(IDENTIFIER)
        self._scan("'{'")
        member_PLUS = self.member_PLUS()
        self._scan("'}'")
        return self.p.struct_type_body(res, member_PLUS)

    def member_PLUS(self):
        member = self.member()
        member_STAR = self.member_STAR()
        return member+member_STAR # member is a list

    def member_STAR(self):
        res = []
        while self._peek() in ['IDENTIFIER', 'SCOPE_DELIMITER', 'STRUCT', 'UNION', 'ENUM', 'FLOAT', 'DOUBLE', 'SHORT', 'LONG', 'UNSIGNED', 'CHAR', 'WCHAR', 'BOOLEAN', 'OCTET', 'IDL_ANY', 'OBJECT', 'SEQUENCE', 'STRING', 'WSTRING', 'FIXED']:
            member = self.member()
            res = res + member
        return res

    def member(self):
        type_spec = self.type_spec()
        declarator_CSV_PLUS = self.declarator_CSV_PLUS()
        self._scan("';'")
        return self.p.member(type_spec, declarator_CSV_PLUS)

    def union_type(self):
        UNION = self._scan('UNION')
        IDENTIFIER = self._scan('IDENTIFIER')
        SWITCH = self._scan('SWITCH')
        LPAREN = self._scan('LPAREN')
        switch_type_spec = self.switch_type_spec()
        RPAREN = self._scan('RPAREN')
        res = self.p.union_type_header(IDENTIFIER, switch_type_spec)
        self._scan("'{'")
        case_PLUS = self.case_PLUS()
        self._scan("'}'")
        return self.p.union_type_body(res, case_PLUS)

    def switch_type_spec(self):
        _token_ = self._peek()
        if _token_ in ['SHORT', 'LONG', 'UNSIGNED']:
            integer_type = self.integer_type()
            return integer_type
        elif _token_ == 'CHAR':
            char_type = self.char_type()
            return char_type
        elif _token_ == 'BOOLEAN':
            boolean_type = self.boolean_type()
            return boolean_type
        elif _token_ == 'ENUM':
            enum_type = self.enum_type()
            return enum_type
        elif _token_ in ['IDENTIFIER', 'SCOPE_DELIMITER']:
            scoped_name = self.scoped_name()
            return self.p.switch_type_spec_scoped_name(scoped_name)
        else:
            raise SyntaxError(self._pos, 'Could not match switch_type_spec')

    def case_PLUS(self):
        case = self.case()
        case_STAR = self.case_STAR()
        return self.p.list_insert(case, case_STAR)

    def case_STAR(self):
        _token_ = self._peek()
        if _token_ == "'}'":
            return self.p.list_empty()
        elif _token_ in ['CASE', 'DEFAULT']:
            case = self.case()
            case_STAR = self.case_STAR()
            return self.p.list_insert(case, case_STAR)
        else:
            raise SyntaxError(self._pos, 'Could not match case_STAR')

    def case(self):
        case_label_PLUS = self.case_label_PLUS()
        element_spec = self.element_spec()
        self._scan("';'")
        return self.p.case(case_label_PLUS, element_spec)

    def case_label_PLUS(self):
        case_label = self.case_label()
        case_label_STAR = self.case_label_STAR()
        return self.p.list_insert(case_label, case_label_STAR)

    def case_label_STAR(self):
        _token_ = self._peek()
        if _token_ in ['IDENTIFIER', 'SCOPE_DELIMITER', 'STRUCT', 'UNION', 'ENUM', 'FLOAT', 'DOUBLE', 'SHORT', 'LONG', 'UNSIGNED', 'CHAR', 'WCHAR', 'BOOLEAN', 'OCTET', 'IDL_ANY', 'OBJECT', 'SEQUENCE', 'STRING', 'WSTRING', 'FIXED']:
            return self.p.list_empty()
        elif _token_ in ['CASE', 'DEFAULT']:
            case_label = self.case_label()
            case_label_STAR = self.case_label_STAR()
            return self.p.list_insert(case_label, case_label_STAR)
        else:
            raise SyntaxError(self._pos, 'Could not match case_label_STAR')

    def case_label(self):
        _token_ = self._peek()
        if _token_ == 'CASE':
            CASE = self._scan('CASE')
            const_expr = self.const_expr()
            self._scan("':'")
            return const_expr
        elif _token_ == 'DEFAULT':
            DEFAULT = self._scan('DEFAULT')
            self._scan("':'")
            return self.p.case_label_default()
        else:
            raise SyntaxError(self._pos, 'Could not match case_label')

    def element_spec(self):
        type_spec = self.type_spec()
        declarator = self.declarator()
        return self.p.element_spec(type_spec, declarator)

    def enum_type(self):
        ENUM = self._scan('ENUM')
        IDENTIFIER = self._scan('IDENTIFIER')
        res = self.p.enum_type_header(IDENTIFIER)
        self._scan("'{'")
        enumerator_CSV_PLUS = self.enumerator_CSV_PLUS()
        self._scan("'}'")
        return self.p.enum_type_body(res, enumerator_CSV_PLUS)

    def enumerator_CSV_PLUS(self):
        enumerator = self.enumerator()
        enumerator_CSV_STAR = self.enumerator_CSV_STAR()
        return self.p.list_insert(enumerator, enumerator_CSV_STAR)

    def enumerator_CSV_STAR(self):
        _token_ = self._peek()
        if _token_ == "'}'":
            return self.p.list_empty()
        elif _token_ == "','":
            self._scan("','")
            enumerator = self.enumerator()
            enumerator_CSV_STAR = self.enumerator_CSV_STAR()
            return self.p.list_insert(enumerator, enumerator_CSV_STAR)
        else:
            raise SyntaxError(self._pos, 'Could not match enumerator_CSV_STAR')

    def enumerator(self):
        IDENTIFIER = self._scan('IDENTIFIER')
        return IDENTIFIER

    def sequence_type(self):
        SEQUENCE = self._scan('SEQUENCE')
        self._scan("'<'")
        simple_type_spec = self.simple_type_spec()
        COMMA_positive_int_const_OPT = self.COMMA_positive_int_const_OPT()
        self._scan("'>'")
        return self.p.sequence_type(simple_type_spec, COMMA_positive_int_const_OPT)

    def COMMA_positive_int_const_OPT(self):
        _token_ = self._peek()
        if _token_ == "'>'":
            return 0
        elif _token_ == "','":
            self._scan("','")
            positive_int_const = self.positive_int_const()
            return positive_int_const
        else:
            raise SyntaxError(self._pos, 'Could not match COMMA_positive_int_const_OPT')

    def string_type(self):
        STRING = self._scan('STRING')
        string_bound_OPT = self.string_bound_OPT()
        return self.p.string_type(string_bound_OPT)

    def string_bound_OPT(self):
        _token_ = self._peek()
        if _token_ in ['IDENTIFIER', "'>'", "','", "';'"]:
            return 0
        elif _token_ == "'<'":
            self._scan("'<'")
            positive_int_const = self.positive_int_const()
            self._scan("'>'")
            return positive_int_const
        else:
            raise SyntaxError(self._pos, 'Could not match string_bound_OPT')

    def wide_string_type(self):
        WSTRING = self._scan('WSTRING')
        string_bound_OPT = self.string_bound_OPT()
        return self.p.wide_string_type(string_bound_OPT)

    def array_declarator(self):
        IDENTIFIER = self._scan('IDENTIFIER')
        fixed_array_size_PLUS = self.fixed_array_size_PLUS()
        return self.p.array_declarator(IDENTIFIER, fixed_array_size_PLUS)

    def fixed_array_size_PLUS(self):
        fixed_array_size = self.fixed_array_size()
        fixed_array_size_STAR = self.fixed_array_size_STAR()
        return self.p.list_insert(fixed_array_size, fixed_array_size_STAR)

    def fixed_array_size_STAR(self):
        _token_ = self._peek()
        if _token_ == 'LBRACKET':
            fixed_array_size = self.fixed_array_size()
            fixed_array_size_STAR = self.fixed_array_size_STAR()
            return self.p.list_insert(fixed_array_size, fixed_array_size_STAR)
        else:
            raise SyntaxError(self._pos, 'Could not match fixed_array_size_STAR')

    def fixed_array_size(self):
        LBRACKET = self._scan('LBRACKET')
        positive_int_const = self.positive_int_const()
        RBRACKET = self._scan('RBRACKET')
        return self.p.fixed_array_size(positive_int_const)

    def attr_dcl(self):
        readonly_OPT = self.readonly_OPT()
        ATTRIBUTE = self._scan('ATTRIBUTE')
        param_type_spec = self.param_type_spec()
        simple_declarator_CSV_PLUS = self.simple_declarator_CSV_PLUS()
        self.p.attr_dcl(readonly_OPT, param_type_spec, simple_declarator_CSV_PLUS)

    def readonly_OPT(self):
        _token_ = self._peek()
        if _token_ == 'ATTRIBUTE':
            return None
        elif _token_ == 'READONLY':
            READONLY = self._scan('READONLY')
            return self.p.READONLY
        else:
            raise SyntaxError(self._pos, 'Could not match readonly_OPT')

    def simple_declarator_CSV_PLUS(self):
        simple_declarator = self.simple_declarator()
        simple_declarator_CSV_STAR = self.simple_declarator_CSV_STAR()
        return self.p.list_insert(simple_declarator, simple_declarator_CSV_STAR)

    def simple_declarator_CSV_STAR(self):
        _token_ = self._peek()
        if _token_ == "';'":
            return self.p.list_empty()
        elif _token_ == "','":
            self._scan("','")
            simple_declarator = self.simple_declarator()
            simple_declarator_CSV_STAR = self.simple_declarator_CSV_STAR()
            return self.p.list_insert(simple_declarator, simple_declarator_CSV_STAR)
        else:
            raise SyntaxError(self._pos, 'Could not match simple_declarator_CSV_STAR')

    def except_dcl(self):
        EXCEPTION = self._scan('EXCEPTION')
        IDENTIFIER = self._scan('IDENTIFIER')
        res = self.p.except_dcl_header(IDENTIFIER)
        self._scan("'{'")
        member_STAR = self.member_STAR()
        self._scan("'}'")
        self.p.except_dcl_body(res, member_STAR)

    def op_dcl(self):
        op_attribute_OPT = self.op_attribute_OPT()
        op_type_spec = self.op_type_spec()
        IDENTIFIER = self._scan('IDENTIFIER')
        res = self.p.op_dcl_header(op_attribute_OPT, op_type_spec, IDENTIFIER)
        parameter_dcls = self.parameter_dcls()
        raises_expr_OPT = self.raises_expr_OPT()
        context_expr_OPT = self.context_expr_OPT()
        self.p.op_dcl_body(res, parameter_dcls, raises_expr_OPT, context_expr_OPT)

    def op_attribute_OPT(self):
        _token_ = self._peek()
        if _token_ in ['VOID', 'STRING', 'WSTRING', 'IDENTIFIER', 'SCOPE_DELIMITER', 'FLOAT', 'DOUBLE', 'SHORT', 'LONG', 'UNSIGNED', 'CHAR', 'WCHAR', 'BOOLEAN', 'OCTET', 'IDL_ANY', 'OBJECT', 'FIXED']:
            return None
        elif _token_ == 'ONEWAY':
            ONEWAY = self._scan('ONEWAY')
            return self.p.ONEWAY
        else:
            raise SyntaxError(self._pos, 'Could not match op_attribute_OPT')

    def op_type_spec(self):
        _token_ = self._peek()
        if _token_ in ['STRING', 'WSTRING', 'IDENTIFIER', 'SCOPE_DELIMITER', 'FLOAT', 'DOUBLE', 'SHORT', 'LONG', 'UNSIGNED', 'CHAR', 'WCHAR', 'BOOLEAN', 'OCTET', 'IDL_ANY', 'OBJECT', 'FIXED']:
            param_type_spec = self.param_type_spec()
            return param_type_spec
        elif _token_ == 'VOID':
            VOID = self._scan('VOID')
            return self.p.VOID
        else:
            raise SyntaxError(self._pos, 'Could not match op_type_spec')

    def parameter_dcls(self):
        LPAREN = self._scan('LPAREN')
        param_dcls = self.param_dcls()
        RPAREN = self._scan('RPAREN')
        return param_dcls

    def param_dcls(self):
        _token_ = self._peek()
        if _token_ == 'RPAREN':
            return []
        elif _token_ in ['IN', 'OUT', 'INOUT']:
            param_dcl = self.param_dcl()
            res = [param_dcl]
            while self._peek() == "','":
                self._scan("','")
                param_dcl = self.param_dcl()
                res.append(param_dcl)
            return res
        else:
            raise SyntaxError(self._pos, 'Could not match param_dcls')

    def param_dcl(self):
        param_attribute = self.param_attribute()
        param_type_spec = self.param_type_spec()
        simple_declarator = self.simple_declarator()
        return self.p.param_dcl(param_attribute, param_type_spec, simple_declarator)

    def param_attribute(self):
        _token_ = self._peek()
        if _token_ == 'IN':
            IN = self._scan('IN')
            return self.p.IN
        elif _token_ == 'OUT':
            OUT = self._scan('OUT')
            return self.p.OUT
        elif _token_ == 'INOUT':
            INOUT = self._scan('INOUT')
            return self.p.INOUT
        else:
            raise SyntaxError(self._pos, 'Could not match param_attribute')

    def raises_expr_OPT(self):
        _token_ = self._peek()
        if _token_ in ['CONTEXT', "';'"]:
            return None
        elif _token_ == 'RAISES':
            raises_expr = self.raises_expr()
            return raises_expr
        else:
            raise SyntaxError(self._pos, 'Could not match raises_expr_OPT')

    def raises_expr(self):
        RAISES = self._scan('RAISES')
        LPAREN = self._scan('LPAREN')
        scoped_name_CSV_PLUS = self.scoped_name_CSV_PLUS()
        RPAREN = self._scan('RPAREN')
        return scoped_name_CSV_PLUS

    def context_expr_OPT(self):
        _token_ = self._peek()
        if _token_ == "';'":
            return None
        elif _token_ == 'CONTEXT':
            context_expr = self.context_expr()
            return context_expr
        else:
            raise SyntaxError(self._pos, 'Could not match context_expr_OPT')

    def context_expr(self):
        CONTEXT = self._scan('CONTEXT')
        LPAREN = self._scan('LPAREN')
        string_literal_CSV_PLUS = self.string_literal_CSV_PLUS()
        RPAREN = self._scan('RPAREN')
        return self.p.context_expr(string_literal_CSV_PLUS)

    def string_literal_CSV_PLUS(self):
        STRING_LITERAL = self._scan('STRING_LITERAL')
        string_literal_CSV_STAR = self.string_literal_CSV_STAR()
        return self.p.list_insert(STRING_LITERAL, string_literal_CSV_STAR)

    def string_literal_CSV_STAR(self):
        _token_ = self._peek()
        if _token_ == 'RPAREN':
            return self.p.list_empty()
        elif _token_ == "','":
            self._scan("','")
            STRING_LITERAL = self._scan('STRING_LITERAL')
            string_literal_CSV_STAR = self.string_literal_CSV_STAR()
            return self.p.list_insert(STRING_LITERAL, string_literal_CSV_STAR)
        else:
            raise SyntaxError(self._pos, 'Could not match string_literal_CSV_STAR')

    def param_type_spec(self):
        _token_ = self._peek()
        if _token_ in ['FLOAT', 'DOUBLE', 'SHORT', 'LONG', 'UNSIGNED', 'CHAR', 'WCHAR', 'BOOLEAN', 'OCTET', 'IDL_ANY', 'OBJECT']:
            base_type_spec = self.base_type_spec()
            return base_type_spec
        elif _token_ == 'STRING':
            string_type = self.string_type()
            return string_type
        elif _token_ == 'WSTRING':
            wide_string_type = self.wide_string_type()
            return wide_string_type
        elif _token_ == 'FIXED':
            fixed_pt_type = self.fixed_pt_type()
            return fixed_pt_type
        elif _token_ in ['IDENTIFIER', 'SCOPE_DELIMITER']:
            scoped_name = self.scoped_name()
            return self.p.idl_type_scoped_name(scoped_name)
        else:
            raise SyntaxError(self._pos, 'Could not match param_type_spec')

    def fixed_pt_type(self):
        FIXED = self._scan('FIXED')
        self._scan("'<'")
        positive_int_const = self.positive_int_const()
        self._scan("','")
        integer_literal = self.integer_literal()
        self._scan("'>'")
        return self.p.fixed_pt_type(positive_int_const, integer_literal)

    def fixed_pt_const_type(self):
        FIXED = self._scan('FIXED')
        return self.p.fixed_pt_const_type()


def parse(rule, text):
    P = IDL(IDLScanner(text))
    return wrap_error_reporter(P, rule)




OrigIDLScanner = IDLScanner
class IDLScanner(OrigIDLScanner):
    YYERROR = SyntaxError
    def __init__(self, text):
        OrigIDLScanner.__init__(self, text)
        self.yylineno_pos = 0
        self.yylineno_start = 0
        
    def yylineno(self, newline = None):
        if newline is not None:
            self.yylineno_pos = self.pos
            self.yylineno_start = newline
        return self.yylineno_start+count(self.input,'\n',
                                         self.yylineno_pos,self.pos)

    def yyfileandlineno(self):
        # Not supported in this scanner
        return self.yylineno()

    def scan(self, restrict):
        if restrict:
            restrict = restrict + ['LINE']
        while 1:
            OrigIDLScanner.scan(self, restrict)
            if self.tokens[-1][2] == 'LINE':
                result = self.tokens.pop()
                self.restrictions.pop()
                self.p.line_directive(result[3])
                continue
            break

class IDLParser(IDL):
    def do_declarator(self, ident, arrays):
        if arrays:
            return self.p.array_declarator(ident, arrays)
        else:
            return self.p.simple_declarator(ident)

    def do_long_long(self, unsigned, long_long):
        if unsigned:
            if long_long:
                return self.p.unsigned_longlong_int()
            else:
                return self.p.UNSIGNED_LONG_INT
        else:
            if long_long:
                return self.p.signed_longlong_int()
            else:
                return self.p.SIGNED_LONG_INT

inv_keywords = {
    "ABSTRACT":     "abstract",
    "IDL_ANY":      "any",
    "ATTRIBUTE":    "attribute",
    "BOOLEAN":      "boolean",
    "CASE":         "case",
    "CHAR":         "char",
    "CONST":        "const",
    "CONTEXT":      "context",
    "CUSTOM":       "custom",
    "DEFAULT":      "default",
    "DOUBLE":       "double",
    # alphabetic order error in CORBA 2.6
    "ENUM":         "enum",
    "EXCEPTION":    "exception",
    "FACTORY":      "factory",
    "IDL_FALSE":    "FALSE",
    "FIXED":        "fixed",
    "FLOAT":        "float",
    "IN":           "in",
    "INOUT":        "inout",
    "INTERFACE":    "interface",
    "LOCAL":        "local",
    "LONG":         "long",
    "MODULE":       "module",
    "NATIVE":       "native",
    "OBJECT":       "Object",
    "OCTET":        "octet",
    "ONEWAY":       "oneway",
    "OUT":          "out",
    "PRIVATE":      "private",
    "PUBLIC":       "public",
    "RAISES":       "raises",
    "READONLY":     "readonly",
    "SEQUENCE":     "sequence",
    "SHORT":        "short",
    "STRING":       "string",
    "STRUCT":       "struct",
    "SUPPORTS":     "supports",
    "SWITCH":       "switch",
    "IDL_TRUE":     "TRUE",
    "TRUNCATABLE":  "truncatable",
    "TYPEDEF":      "typedef",
    # alphabetic order error in CORBA 2.6
    "UNION":        "union",
    "UNSIGNED":     "unsigned",
    "VALUEBASE":    "ValueBase",
    "VALUETYPE":    "valuetype",
    "VOID":         "void",
    "WCHAR":        "wchar",
    "WSTRING":      "wstring",
    }

keywords = {}
for k, v in inv_keywords.items():
    keywords[v] = k

# named punctuators
inv_puncs = {
    "LEFT_SHIFT":                   '<<',
    "RIGHT_SHIFT":                  '>>',
    "LBRACKET":                     '[',
    "RBRACKET":                     ']',
    "LPAREN":                       '(',
    "RPAREN":                       ')',
    "PLUS":                         '+',
    "STAR":                         '*',
    "BAR":                          '|',
    "CARET":                        '^',
    "SCOPE_DELIMITER":              '::',
}

puncs = {}
for k,v in inv_puncs.items():
    puncs[v] = k

class TokenizedScanner:
    YYERROR = SyntaxError
    def __init__(self, preprocessor):
        from cpp import IDENTIFIER, NUMBER, CHARLITERAL, \
             STRINGLITERAL, PREPROCESSING_OP_OR_PUNC, PRAGMA, \
             INCLUDE_START, INCLUDE_END, OTHER, SyntaxError
        self.tokens = []
        self.pos = 0
        while 1:
            t = preprocessor.get_token()
            if t is None:
                self.tokens.append((-1, None, 'END', '$'))
                return
            pos = t.file, t.line
            text = t.text
            if t.type == IDENTIFIER:
                try:
                    new_type = keywords[t.text]
                except KeyError:
                    if text[0] == '_':
                        text = text[1:]
                    new_type = "IDENTIFIER"
            elif t.type == NUMBER:
                if re.search("[.eE]", text):
                    new_type = "FLOATING_PT_LITERAL"
                else:
                    new_type = "INTEGER_LITERAL"
                # Peek next token to see whether it is a "D"
                if not t.space_follows:
                    t1 = preprocessor.get_token()
                    if t1 and t1.text in ['d','D']:
                        text += t1.text
                        new_type = FIXED_PT_LITERAL
                    elif t1:
                        preprocessor.unget_token(t1)
            elif t.type == CHARLITERAL:
                if text[0] == 'L':
                    new_type = 'WIDE_CHARACTER_LITERAL'
                else:
                    new_type = 'CHARACTER_LITERAL'
            elif t.type == STRINGLITERAL:
                if text[0] == 'L':
                    new_type = 'WIDE_STRING_LITERAL'
                else:
                    new_type = 'STRING_LITERAL'
            elif t.type == PREPROCESSING_OP_OR_PUNC:
                try:
                    new_type = puncs[t.text]
                except KeyError:
                    # Literal tokens are quoted
                    new_type = repr(t.text)
            elif t.type == PRAGMA:
                # XXX this is already tokenized in t.pragma
                text = '#pragma ' + text
                new_type = 'PRAGMA'
            elif t.type == INCLUDE_START:
                # XXX: inform parser
                continue
            elif t.type == INCLUDE_END:
                # XXX: inform parser
                continue
            elif t.type == OTHER:
                # Should not happen - complain right here
                raise SyntaxError("Unexpected token '\\x%.2x'" % ord(text))
            else:
                assert 0
            self.tokens.append((pos, None, new_type, text))

    def token(self, i, restrict = 0):
        self.pos = i
        try:
            return self.tokens[i]
        except IndexError:
            raise NoMoreTokens()

    def yyfileandlineno(self):
        return self.tokens[self.pos][0]

    def yylineno(self):
        return self.yyfileandlineno()[1]

    def __repr__(self):
        res = []
        for t in self.tokens[max(self.pos-5,0):self.pos+1]:
            res.append(t[3])
        return repr(res)

