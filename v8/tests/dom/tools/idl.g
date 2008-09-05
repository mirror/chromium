
%%
parser IDL:
    option:             'context-insensitive-scanner'
    ignore:             "[ \n\r\t]+"
    ignore:             "#[ \t]*ident.*\n"
    ignore:             "//.*\n"
    token PRAGMA:       "#[ \t]*pragma.*\n|#[\ \t]*[0-9]+[ \t]*pragma.*\n"
    token LINE:         "#[ \t]*[0-9].*\n|#[ \t]*line.*\n"
    # The list of keywords is duplicated at the bottom of the file
    token ABSTRACT:     "abstract"
    token IDL_ANY:      "any"
    token ATTRIBUTE:    "attribute"
    token BOOLEAN:      "boolean"
    token CASE:         "case"
    token CHAR:         "char"
    token CONST:        "const"
    token CONTEXT:      "context"
    token CUSTOM:       "custom"
    token DEFAULT:      "default"
    token DOUBLE:       "double"
    # alphabetic order error in CORBA 2.6
    token ENUM:         "enum"
    token EXCEPTION:    "exception"
    token FACTORY:      "factory"
    token IDL_FALSE:    "FALSE"
    token FIXED:        "fixed"
    token FLOAT:        "float"
    token IN:           "in"
    token INOUT:        "inout"
    token INTERFACE:    "interface"
    token LOCAL:        "local"
    token LONG:         "long"
    token MODULE:       "module"
    token NATIVE:       "native"
    token OBJECT:       "Object"
    token OCTET:        "octet"
    token ONEWAY:       "oneway"
    token OUT:          "out"
    token PRIVATE:      "private"
    token PUBLIC:       "public"
    token RAISES:       "raises"
    token READONLY:     "readonly"
    token SCOPE_DELIMITER:      "::"
    token SEQUENCE:     "sequence"
    token SHORT:        "short"
    token STRING:       "string"
    token STRUCT:       "struct"
    token SUPPORTS:     "supports"
    token SWITCH:       "switch"
    token IDL_TRUE:     "TRUE"
    token TRUNCATABLE:  "truncatable"
    token TYPEDEF:      "typedef"
    # alphabetic order error in CORBA 2.6
    token UNION:        "union"
    token UNSIGNED:     "unsigned"
    token VALUEBASE:    "ValueBase"
    token VALUETYPE:    "valuetype"
    token VOID:         "void"
    token WCHAR:        "wchar"
    token WSTRING:      "wstring"

    token IDENTIFIER:                   "[_a-zA-Z][a-zA-Z0-9_]*"
    token CHARACTER_LITERAL:            "'.'"
    token WIDE_CHARACTER_LITERAL:       "L'.'"
    token FLOATING_PT_LITERAL:          r"([0-9]+\.[0-9]*|\.[0-9]+)([eE][+-]?[0-9]+)?|[0-9]+[eE][+-]?[0-9]+"
    token INTEGER_LITERAL:              "(0|0x)?[0-9a-fA-F]+"
    token STRING_LITERAL:               r'"([^\\"]+|\\.)*"'
    token WIDE_STRING_LITERAL:          r'L"([^\\"]+|\\.)*"'
    token FIXED_PT_LITERAL:             r'[0-9]+\.[0-9]*D'

    token LEFT_SHIFT:                   '<<'
    token RIGHT_SHIFT:                  '>>'
    token LBRACKET:                     r'\['
    token RBRACKET:                     r'\]'
    token LPAREN:                       r'\('
    token RPAREN:                       r'\)'
    token PLUS:                         r'\+'
    token STAR:                         r'\*'
    token BAR:                          r'\|'
    token CARET:                        r'\^'
    token END:                          r'\$'

    rule start:
                        END
                        |
                        specification END


    rule specification: {{

                        }}
                        definition+
                        {{

                        }}

   rule definition:	    type_dcl ';'
                        |
                        const_dcl ';'
                        |
                        except_dcl ';'
                        |
                        interface ';'
                        |
                        module ';'
                        |
                        value ';'
                        |
                        ABSTRACT abstract_thing ';'
                        |
                        PRAGMA
                        {{
			    self.p.pragma(PRAGMA)
                        }}

    rule module:	MODULE IDENTIFIER
                        {{
			    self.p.module_header(IDENTIFIER)
			}}
                        '{' definition+ '}'
                        {{
			    self.p.module_body()
			}}

    rule abstract_thing:
                        INTERFACE IDENTIFIER
                        interface_body_opt<<("abstract", IDENTIFIER)>>
                        {{
                            raise NotImplementedError
                        }}
                        |
                        VALUETYPE IDENTIFIER
                        value_inheritance_spec
                        {{
			               res = self.p.value_dcl_header(("abstract", IDENTIFIER), 
                                          value_inheritance_spec)
			            }}
                        '{'
                            export *
                        '}'
                        {{
                           self.p.value_dcl_body(res)
                        }}

    rule interface:     local_opt INTERFACE IDENTIFIER 
                        interface_body_opt<<(local_opt, IDENTIFIER)>>

    rule local_opt:
                        {{
                            return None
                        }}
                        |
                        LOCAL
                        {{
                            return "local"
                        }}

    rule interface_body_opt<<ident>>:
                        {{
                            self.p.foward_dcl(ident)
                        }}
                        |
                        interface_inheritance_spec
                        {{
			    res = self.p.interface_dcl_header(ident, interface_inheritance_spec)
			}}
                        '{' export* '}'
                        {{
			    self.p.interface_dcl_body(res)
			}}

    rule export:			type_dcl ';'
                        |
                        const_dcl ';'
                        |
                        except_dcl ';'
                        |
                        attr_dcl ';'
                        |
                        op_dcl ';'
                        |
                        PRAGMA
                        {{
			    self.p.pragma(PRAGMA)
                        }}

    rule interface_inheritance_spec:
                        {{
			    return []
                        }}
                        |
			':' scoped_name_CSV_PLUS
                        {{
			    return scoped_name_CSV_PLUS
			}}

    rule scoped_name_CSV_PLUS:   scoped_name scoped_name_CSV
                        {{
                            return self.p.list_insert(scoped_name, scoped_name_CSV)
			}}


    rule scoped_name_CSV:
                        {{
			    return self.p.list_empty()
			}}
                        |
			',' scoped_name scoped_name_CSV
                        {{
			    return self.p.list_insert(scoped_name, scoped_name_CSV)
			}}


    rule scoped_name_0:	IDENTIFIER          {{ return IDENTIFIER }}
                        |
                        SCOPE_DELIMITER IDENTIFIER
                        {{
                            return self.p.scoped_name_absolute(IDENTIFIER)
                        }}


    rule scoped_name:   scoped_name_0
                        {{
                            res = scoped_name_0
                        }}
                        ( SCOPE_DELIMITER IDENTIFIER
                          {{
                             res = self.p.scoped_name_relative(res, IDENTIFIER)
                          }}
                        )*
                        {{
                            return res
			}}

    rule value:         value_dcl
                        |
                        custom_value_dcl

    rule value_dcl:    VALUETYPE IDENTIFIER 
                       value_body<<(None, IDENTIFIER)>>

    rule custom_value_dcl:
                       CUSTOM VALUETYPE IDENTIFIER 
                       value_body<<("custom",IDENTIFIER)>>

    rule value_body<<ident>>:
                       {{
                          return self.p.value_forward_dcl(ident)
                       }}
                       |
                       type_spec
                       {{
                          return self.p.value_box_dcl(ident,type_spec)
                       }}
                       |
                       value_inheritance_spec
                        {{
			               res = self.p.value_dcl_header(ident, 
                                          value_inheritance_spec)
			            }}
                       '{'
                          value_element<<res>>*
                       '}'
                        {{
                           self.p.value_dcl_body(res)
                        }}

    rule value_inheritance_spec:
                        value_inherits_opt
                        value_supports_opt
                        {{
                            return (value_inherits_opt, value_supports_opt)
                        }}

    rule value_inherits_opt:
                        {{
                            return None
                        }}
                        |
                        ':' truncatable_opt scoped_name_CSV_PLUS
                        {{
                            return (truncatable_opt, scoped_name_CSV_PLUS)
                        }}

    rule truncatable_opt:
		                {{
                            return None
                        }}
                        |
                        TRUNCATABLE
                        {{
                            return "truncatable"
                        }}

    rule value_supports_opt:
                        {{
                            return None
                        }}
                        |
                        SUPPORTS scoped_name_CSV_PLUS
                        {{
                            return scoped_name_CSV_PLUS
                        }}

    rule value_element<<value>>:
                        export
                        |
                        state_member
                        |
                        init_dcl<<value>>

    rule state_member:
                        public_private type_spec declarator_CSV_PLUS ';'
                        {{
                            return self.p.state_member(public_private, 
                                                       type_spec,
                                                       declarator_CSV_PLUS)
                        }}

    rule public_private:
                          PUBLIC     {{return "public"}}
                        | PRIVATE    {{return "private"}}

    # directions must be in only
    rule init_dcl<<value>>:      
                        FACTORY IDENTIFIER parameter_dcls ';'
                        {{
                            return self.p.init_dcl(IDENTIFIER, parameter_dcls, value)
                        }}

    rule const_dcl:		CONST const_type IDENTIFIER '=' const_expr
                        {{
			    return self.p.const_dcl(const_type, IDENTIFIER, const_expr)
			}}


    rule const_type:	integer_type        {{return integer_type}}
                        |
                        char_type           {{return char_type}}
                        |
                        wide_char_type      {{return wide_char_type}}
                        |
                        boolean_type        {{return boolean_type}}
                        |
                        floating_pt_type    {{return floating_pt_type}}
                        |
                        string_type         {{return string_type}}
                        |
                        wide_string_type    {{return wide_string_type}}
                        |
                        fixed_pt_const_type {{return fixed_pt_const_type}}
                        |
                        scoped_name
                        {{
			    return self.p.const_type_scoped_name(scoped_name)
			}}


    rule const_expr:    or_expr          {{return or_expr}}


    rule or_expr:       xor_expr
                        {{
                            res = xor_expr
                        }}
                        ( BAR xor_expr
                          {{
	  		    res = self.p.or_expr(res, xor_expr)
		  	  }}
                        )*
                        {{
                            return res
                        }}


    rule xor_expr:      and_expr
                        {{
                            res = and_expr
                        }}
                        ( CARET and_expr
                          {{
	  		    res = self.p.xor_expr(res, and_expr)
		  	  }}
                        )*
                        {{
                            return res
                        }}

    rule and_expr:      shift_expr
                        {{
                            res = shift_expr
                        }}
                        ( '&' shift_expr
                          {{
	  		    res = self.p.and_expr(res, shift_expr)
		  	  }}
                        )*
                        {{
                            return res
                        }}

    rule shift_expr:    add_expr
                        {{
                            res = add_expr
                        }}
                        ( RIGHT_SHIFT add_expr
                          {{
	  		    res = self.p.shift_expr_right(res, add_expr)
                          }}
                          |
                          LEFT_SHIFT add_expr
                          {{
	  		    res = self.p.shift_expr_left(res, add_expr)
		  	  }}
                        )*
                        {{
                            return res
                        }}

    rule add_expr:      mult_expr
                        {{
                            res = mult_expr
                        }}
                        ( PLUS mult_expr
                          {{
	  		    res = self.p.add_expr_add(res, mult_expr)
                          }}
                          |
                          '-' mult_expr
                          {{
	  		    res = self.p.add_expr_subtract(res, mult_expr)
		  	  }}
                        )*
                        {{
                            return res
                        }}

    rule mult_expr:     unary_expr
                        {{
                            res = unary_expr
                        }}
                        ( STAR unary_expr
                          {{
	  		    res = self.p.mult_expr_multiply(res, unary_expr)
                          }}
                          |
                          '/' unary_expr
                          {{
	  		    res = self.p.mult_expr_divide(res, unary_expr)
		  	  }}
                          |
                          '%' unary_expr
                          {{
	  		    res = self.p.mult_expr_mod(res, unary_expr)
		  	  }}
                        )*
                        {{
                            return res
                        }}

    rule unary_expr:             '-' primary_expr
                        {{
			    return self.p.unary_expr_neg(primary_expr)
			}}
                        |
                        PLUS primary_expr 
                        {{
			    return self.p.unary_expr_pos(primary_expr)
			}}
                        |
                        '~' primary_expr
                        {{
			    return self.p.unary_expr_invert(primary_expr)
			}}
                        |
                        primary_expr
                        {{
                            return primary_expr
                        }}


    rule primary_expr:		scoped_name
                        {{
			    return self.p.primary_expr_scoped_name(scoped_name)
			}}
                        |
                        literal
                        {{
                            return literal
                        }}
                        |
                        LPAREN const_expr RPAREN
                        {{
                            return const_expr
                        }}


    rule literal:		integer_literal
                        {{
                            return integer_literal
                        }}
                        |
                        STRING_LITERAL
                        {{
			    return self.p.literal_string_literal(STRING_LITERAL)
			}}
                        |
                        CHARACTER_LITERAL
                        {{
			    return self.p.literal_character_literal(CHARACTER_LITERAL)
			}}
                        |
                        FIXED_PT_LITERAL
                        {{
			    return self.p.literal_fixed_pt_literal(FIXED_PT_LITERAL)
			}}
                        |
                        FLOATING_PT_LITERAL
                        {{
			    return self.p.literal_floating_pt_literal(float(FLOATING_PT_LITERAL))
			}}
                        |
                        boolean_literal
                        {{
                            return boolean_literal
                        }}


    rule integer_literal:        INTEGER_LITERAL
                        {{
			    return INTEGER_LITERAL
			}}


    rule boolean_literal:	IDL_TRUE
                        {{
			    return self.p.boolean_literal_true()
			}}
                        |
                        IDL_FALSE
                        {{
			    return self.p.boolean_literal_false()
			}}


    rule positive_int_const:     const_expr
                        {{
			    return self.p.positive_int_const(const_expr)
			}}


    rule type_dcl:		TYPEDEF type_declarator
                        |
                        struct_type
                        |
                        union_type
                        |
                        enum_type
                        |
                        NATIVE simple_declarator
                        {{
			    self.p.native_type_dcl(simple_declarator)
			}}


    rule type_declarator:	type_spec declarator_CSV_PLUS
                        {{
			    self.p.type_declarator(type_spec, declarator_CSV_PLUS)
			}}


    rule type_spec:		simple_type_spec
                        {{
                            return simple_type_spec
                        }}
                        |
                        constr_type_spec
                        {{
                            return constr_type_spec
                        }}


    rule simple_type_spec:	base_type_spec
                        {{
                            return base_type_spec
                        }}
                        |
                        template_type_spec
                        {{
                            return template_type_spec
                        }}
                        |
                        scoped_name
                        {{
			    return self.p.idl_type_scoped_name(scoped_name)
			}}


    rule base_type_spec:	        floating_pt_type
                        {{
                            return floating_pt_type
                        }}
                        |
                        integer_type
                        {{
                            return integer_type
                        }}
                        |
                        char_type
                        {{
                            return char_type
                        }}
                        |
                        wide_char_type
                        {{
                            return wide_char_type
                        }}
                        |
                        boolean_type
                        {{
                            return boolean_type
                        }}
                        |
                        octet_type
                        {{
                            return octet_type
                        }}
                        |
                        any_type
                        {{
                            return any_type
                        }}
                        |
                        object_type
                        {{
                            return object_type
                        }}


    rule template_type_spec:	sequence_type   {{return sequence_type}}
                        |
                        string_type             {{return string_type}}
                        |
                        wide_string_type        {{return wide_string_type}}
                        |
                        fixed_pt_type           {{return fixed_pt_type}}


    rule constr_type_spec:       struct_type    {{return struct_type}}
                        |
                        union_type              {{return union_type}}
                        |
                        enum_type               {{return enum_type}}


    rule declarator_CSV_PLUS:	declarator declarator_CSV
                        {{
			    return self.p.list_insert(declarator, declarator_CSV)
			}}


    rule declarator_CSV:		
                        {{
			    res = self.p.list_empty()
			}}
                        (',' declarator
                        {{
                            self.p.list_insert(declarator, res)
			}})*
                        {{ return res }}


    rule declarator:	IDENTIFIER
                        {{
                            arrays = []
                        }}
                        ( fixed_array_size
                          {{
                            arrays.append(fixed_array_size)
                          }})*
                        {{
                            return self.do_declarator(IDENTIFIER, arrays)
                        }}

    rule simple_declarator:     IDENTIFIER
                        {{
                                return self.p.simple_declarator(IDENTIFIER)
                        }}


    rule floating_pt_type:	FLOAT
                        {{
			    return self.p.float_type()
			}}
                        |
			DOUBLE
                        {{
			    return self.p.double_type()
			}}


    rule integer_type:	SHORT
                        {{
			    return self.p.signed_short_int()
			}}
                        |
                        LONG (long_long<<0>>
                              {{
                                return long_long
                              }}
                              |
                              DOUBLE
                              {{
                                return self.p.longdouble_type()
                              }}
                              )
                        |
                        UNSIGNED (
                            SHORT
                            {{
                                return self.p.UNSIGNED_SHORT_INT
                            }}
                            |
                            LONG long_long<<1>>
                            {{
                                return self.p.UNSIGNED_LONG_INT
                            }}
                        )

    rule long_long<<unsigned>>:
                        {{
                            return self.do_long_long(unsigned, 0)
                        }}
                        |
                        LONG
                        {{
                            return self.do_long_long(unsigned, 1)
                        }}

    rule char_type:		CHAR
                        {{
			    return self.p.char_type()
			}}


    rule wide_char_type:		WCHAR
                        {{
			    return self.p.wide_char_type()
			}}


    rule boolean_type:		BOOLEAN
                        {{
			    return self.p.BOOLEAN
			}}


    rule octet_type:		OCTET
                        {{
			    return self.p.octet_type()
			}}


    rule any_type:		IDL_ANY
                        {{
			    return self.p.any_type()
			}}


    rule object_type:		OBJECT
                        {{
			    return self.p.object_type()
			}}


    rule struct_type:		STRUCT IDENTIFIER
                        {{
			    res = self.p.struct_type_header(IDENTIFIER)
			}}
                        '{' member_PLUS '}'
                        {{
			    return self.p.struct_type_body(res, member_PLUS)
			}}


    rule member_PLUS:   member member_STAR
                        {{
                            return member+member_STAR # member is a list
                        }}


    rule member_STAR:		
                        {{
                            res = []
                        }}
                        ( member {{ res = res + member }}
                        )*
                        {{
			    return res
			}}

    rule member:			type_spec declarator_CSV_PLUS ';'
                        {{
			    return self.p.member(type_spec, declarator_CSV_PLUS)
			}}



    rule union_type:	UNION IDENTIFIER SWITCH LPAREN switch_type_spec RPAREN
                        {{
			    res = self.p.union_type_header(IDENTIFIER, switch_type_spec)
			}}
                        '{' case_PLUS '}'
                        {{
			    return self.p.union_type_body(res, case_PLUS)
			}}

    # XXX: integer_type includes long double
    rule switch_type_spec:	integer_type   {{return integer_type}}
                        |
                        char_type          {{return char_type}}
                        |
                        boolean_type       {{return boolean_type}}
                        |
                        enum_type          {{return enum_type}}
                        |
                        scoped_name
                        {{
			    return self.p.switch_type_spec_scoped_name(scoped_name)
			}}


    rule case_PLUS:		case case_STAR
                        {{
			    return self.p.list_insert(case, case_STAR)
			}}


    rule case_STAR:		
                        {{
			    return self.p.list_empty()
			}}
                        |
                        case case_STAR
                        {{
			    return self.p.list_insert(case, case_STAR)
			}}
              

    rule case:                   case_label_PLUS element_spec ';'
                        {{
			    return self.p.case(case_label_PLUS, element_spec)
			}}


    rule case_label_PLUS:	case_label case_label_STAR
                        {{
			    return self.p.list_insert(case_label, case_label_STAR)
			}}


    rule case_label_STAR:	
                        {{
			    return self.p.list_empty()
			}}
                        |
                        case_label case_label_STAR
                        {{
			    return self.p.list_insert(case_label, case_label_STAR)
			}}


    rule case_label:		CASE const_expr ':'
                        {{
			    return const_expr
			}}
                        |
                        DEFAULT ':'
                        {{
			    return self.p.case_label_default()
			}}


    rule element_spec:           type_spec declarator
                        {{
			    return self.p.element_spec(type_spec, declarator)
			}}


    rule enum_type:		ENUM IDENTIFIER
                        {{
			    res = self.p.enum_type_header(IDENTIFIER)
			}}
                        '{' enumerator_CSV_PLUS '}'
                        {{
			    return self.p.enum_type_body(res, enumerator_CSV_PLUS)
			}}


    rule enumerator_CSV_PLUS:	enumerator enumerator_CSV_STAR
                        {{
			    return self.p.list_insert(enumerator, enumerator_CSV_STAR)
			}}


    rule enumerator_CSV_STAR:	
                        {{
			    return self.p.list_empty()
			}}
                        |
                        ',' enumerator enumerator_CSV_STAR
                        {{
			    return self.p.list_insert(enumerator, enumerator_CSV_STAR)
			}}


    rule enumerator:		IDENTIFIER {{return IDENTIFIER}}


    rule sequence_type:	        SEQUENCE '<'simple_type_spec COMMA_positive_int_const_OPT '>'
                        {{
			    return self.p.sequence_type(simple_type_spec, COMMA_positive_int_const_OPT)
			}}

    rule COMMA_positive_int_const_OPT:
                        {{
                            return 0
                        }}
                        |
                        ',' positive_int_const
                        {{
                            return positive_int_const
                        }}

    rule string_type:		STRING string_bound_OPT
                        {{
			    return self.p.string_type(string_bound_OPT)
			}}

    rule string_bound_OPT:
                        {{
                            return 0
                        }}
                        |
                        '<' positive_int_const '>'
                        {{
                            return positive_int_const
                        }}
                        

    rule wide_string_type:	WSTRING string_bound_OPT
                        {{
			    return self.p.wide_string_type(string_bound_OPT)
			}}

    rule array_declarator:	IDENTIFIER fixed_array_size_PLUS
                        {{
			    return self.p.array_declarator(IDENTIFIER, fixed_array_size_PLUS)
			}}


    rule fixed_array_size_PLUS:	fixed_array_size fixed_array_size_STAR
                        {{
			    return self.p.list_insert(fixed_array_size, fixed_array_size_STAR)
			}}


    rule fixed_array_size_STAR:  
                        {{
			    return self.p.list_empty()
			}}
                        |
			fixed_array_size fixed_array_size_STAR
                        {{
			    return self.p.list_insert(fixed_array_size, fixed_array_size_STAR)
			}}


    rule fixed_array_size:	LBRACKET positive_int_const RBRACKET
                        {{
			    return self.p.fixed_array_size(positive_int_const)
			}}


    rule attr_dcl:		readonly_OPT ATTRIBUTE param_type_spec
                        simple_declarator_CSV_PLUS
                        {{
			    self.p.attr_dcl(readonly_OPT, param_type_spec, simple_declarator_CSV_PLUS)
			}}

    rule readonly_OPT:		
                        {{
			    return None
			}}
                        |
                        READONLY
                        {{
			    return self.p.READONLY
			}}


    rule simple_declarator_CSV_PLUS: simple_declarator simple_declarator_CSV_STAR
                          {{
			      return self.p.list_insert(simple_declarator, simple_declarator_CSV_STAR)
			  }}


    rule simple_declarator_CSV_STAR: 
                          {{
			      return self.p.list_empty()
			  }}
                          |
                          ',' simple_declarator simple_declarator_CSV_STAR
                          {{
			      return self.p.list_insert(simple_declarator, simple_declarator_CSV_STAR)
			  }}


    rule except_dcl:		EXCEPTION IDENTIFIER
                        {{
			    res = self.p.except_dcl_header(IDENTIFIER)
			}}
                        '{' member_STAR '}'
                        {{
			    self.p.except_dcl_body(res, member_STAR)
			}}


    rule op_dcl:			op_attribute_OPT op_type_spec IDENTIFIER
                        {{
			    res = self.p.op_dcl_header(op_attribute_OPT, op_type_spec, IDENTIFIER)
			}}
			parameter_dcls raises_expr_OPT context_expr_OPT
                        {{
			    self.p.op_dcl_body(res, parameter_dcls, raises_expr_OPT, context_expr_OPT)
			}}


    rule op_attribute_OPT:	
                        {{
			    return None
			}}
                        |
                        ONEWAY
                        {{
			    return self.p.ONEWAY
			}}


    rule op_type_spec:		param_type_spec
                        {{
                            return param_type_spec
                        }}
                        |
                        VOID
                        {{
			    return self.p.VOID
			}}


    rule parameter_dcls:
                        LPAREN param_dcls RPAREN
                        {{
			    return param_dcls
			}}

    rule param_dcls:
                        {{
                            return []
                        }}
                        |
                        param_dcl
                        {{
                            res = [param_dcl]
                        }}
                        ( ',' param_dcl
                          {{
                            res.append(param_dcl)
                          }}
                        )*
                        {{
                            return res
                        }}

    rule param_dcl:		param_attribute param_type_spec simple_declarator
                        {{
			    return self.p.param_dcl(param_attribute, param_type_spec, simple_declarator)
			}}


    rule param_attribute:	IN
                        {{
			    return self.p.IN
			}}
                        |
                        OUT
                        {{
			    return self.p.OUT
			}}
                        |
                        INOUT
                        {{
			    return self.p.INOUT
			}}


    rule raises_expr_OPT:	
                        {{
			    return None
			}}
                        |
                        raises_expr
                        {{
                            return raises_expr
                        }}


    rule raises_expr:		RAISES LPAREN scoped_name_CSV_PLUS RPAREN
                        {{
			    return scoped_name_CSV_PLUS
			}}


    rule context_expr_OPT:	
                        {{
			    return None
			}}
                        |
                        context_expr
                        {{
                            return context_expr
                        }}


    rule context_expr:		CONTEXT LPAREN string_literal_CSV_PLUS RPAREN
                        {{
			    return self.p.context_expr(string_literal_CSV_PLUS)
                        }}


    rule string_literal_CSV_PLUS:STRING_LITERAL string_literal_CSV_STAR
                        {{
			    return self.p.list_insert(STRING_LITERAL, string_literal_CSV_STAR)
			}}

    rule string_literal_CSV_STAR:
                        {{
			    return self.p.list_empty()
			}}
                        |
                        ',' STRING_LITERAL string_literal_CSV_STAR
                        {{
			    return self.p.list_insert(STRING_LITERAL, string_literal_CSV_STAR)
		        }}


    rule param_type_spec:        base_type_spec  {{return base_type_spec}}
                        |
                        string_type              {{return string_type}}
                        |
                        wide_string_type         {{return wide_string_type}}
                        |
                        fixed_pt_type            {{return fixed_pt_type}}
                        |
                        scoped_name
                        {{
			    return self.p.idl_type_scoped_name(scoped_name)
			}}


    rule fixed_pt_type:		FIXED '<' positive_int_const ',' integer_literal '>'
                        {{
			    return self.p.fixed_pt_type(positive_int_const, integer_literal)
			}}


    rule fixed_pt_const_type:    FIXED
                        {{
			    return self.p.fixed_pt_const_type()
			}}

%%

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

