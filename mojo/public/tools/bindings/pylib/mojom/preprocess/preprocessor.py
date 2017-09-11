# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Minimal C-style preprocessor that only supports buildflags and OS defines."""

import imp
import os.path
import re
import sys

from ..error import Error


def _GetDirAbove(dirname):
  """Returns the directory "above" this file containing |dirname| (which must
  also be "above" this file)."""
  path = os.path.abspath(__file__)
  while True:
    path, tail = os.path.split(path)
    assert tail
    if tail == dirname:
      return path


try:
  imp.find_module('ply')
except ImportError:
  sys.path.append(os.path.join(_GetDirAbove('mojo'), 'third_party'))
from ply.lex import TOKEN
from ply import lex
from ply import yacc

_COMMENT_RE = re.compile(r'\s*//.*$')
_IF_RE = re.compile(r'#if (.+)$')
_ELIF_RE = re.compile(r'#elif (.+)$')


class LexError(Error):

  def __init__(self, filename, message, lineno):
    Error.__init__(self, filename, message, lineno=lineno)


class ParseError(Error):

  def __init__(self, filename, message, lineno):
    Error.__init__(self, filename, message, lineno=lineno)


# Simple lexer and parser for preprocessor conditions.
class ConditionalLexer(object):

  def __init__(self, filename, lineno):
    self.filename = filename
    self.lineno = lineno

  keywords = {
      'defined': 'DEFINED',
      'BUILDFLAG': 'BUILDFLAG',
  }

  tokens = [
      'OR',
      'AND',
      'NOT',
      'LPAREN',
      'RPAREN',
      'NAME',
  ] + list(keywords.values())

  t_ignore = ' \t\r'

  t_OR = r'\|\|'
  t_AND = r'&&'
  t_NOT = r'!'
  t_LPAREN = r'\('
  t_RPAREN = r'\)'

  def t_NAME(self, t):
    r'[a-zA-Z_][0-9a-zA-Z_]*'
    t.type = self.keywords.get(t.value, 'NAME')
    return t

  def t_error(self, t):
    raise LexError(self.filename,
                   'Unknown token in preprocessor directive: %s' % t.value,
                   self.lineno)


class ConditionalParser(object):

  def __init__(self, filename, lineno, tokens, buildflags, defines):
    self.filename = filename
    self.lineno = lineno
    self.tokens = tokens
    self.buildflags = buildflags
    self.defines = defines

  precedence = (
      ('left', 'OR'),
      ('left', 'AND'),
      ('right', 'NOT'),)

  def p_expression_not(self, p):
    """expression : NOT expression"""
    p[0] = not p[2]

  def p_expression_and(self, p):
    """expression : expression AND expression"""
    p[0] = p[1] and p[3]

  def p_expression_or(self, p):
    """expression : expression OR expression"""
    p[0] = p[1] or p[3]

  def p_paren(self, p):
    """expression : LPAREN expression RPAREN"""
    p[0] = p[2]

  def p_buildflag(self, p):
    """expression : BUILDFLAG LPAREN NAME RPAREN"""
    p[0] = p[3] in self.buildflags and self.buildflags[p[3]]

  def p_defined(self, p):
    """expression : DEFINED LPAREN NAME RPAREN"""
    p[0] = p[3] in self.defines

  def p_error(self, p):
    raise ParseError(self.filename,
                     'Unexpected token in preprocessor directive: %s' % p.value,
                     self.lineno)


def _ProcessConditional(filename, lineno, expression, buildflags, defines):
  """Evaluates a C preprocessor conditional.

  Note this function only supports a limited subset of operators:
  - boolean operators: &&, ||, and !
  - defined()
  - BUILDFLAG()
  """
  conditional_lexer = ConditionalLexer(filename, lineno)
  lexer = lex.lex(object=conditional_lexer)
  parser = yacc.yacc(module=ConditionalParser(
      filename, lineno, conditional_lexer.tokens, buildflags, defines))
  return parser.parse(expression, lexer=lexer)


def PreprocessSource(input_source, filename, buildflags, defines):
  """Performs simple preprocessing on source.

  filename: relative path to the preprocessed file for error messages.
  source: source for a mojom. Lines should be delimited only with \n.
  buildflags: a list of build flags that are enabled.
  defines: a list of defines that should be treated as true.
  """
  # Allow \ to continue long lines. Ideally, this would be limited to just
  # preprocessor directives but it's simpler to just do everything.
  input_source = input_source.replace('\\\n', '')
  lines = input_source.splitlines()
  output_lines = []
  # Tracks whether or not the current preprocessor "block" should emit output.
  # By definition, the top-level block always emits.
  emit_stack = [True]
  lineno = 0
  for line in lines:
    lineno += 1
    # Not a preprocessor directive: just append the line (if output isn't
    # suppressed).
    if not line.startswith('#'):
      if emit_stack[-1]:
        output_lines.append(line)
      continue

    # Strip out comments.
    line = _COMMENT_RE.sub('', line)

    if line == '#endif':
      emit_stack.pop()
      if not emit_stack:
        raise ParseError(filename, 'Found #endif without matching #if', lineno)
      continue

    # Look for a conditional to evaluate.
    match = _IF_RE.match(line)
    if not match:
      match = _ELIF_RE.match(line)
    if match:
      emit_stack.append(
          _ProcessConditional(filename, lineno,
                              match.group(1), buildflags, defines))
      continue

    if line == '#else':
      if len(emit_stack) == 1:
        raise ParseError(filename, 'Found #else without matching #if', lineno)
      emit_stack[-1] = not emit_stack[-1]
      continue
  return '\n'.join(output_lines)
