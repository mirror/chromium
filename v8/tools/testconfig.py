# Test configuration utilities.  This library provides support for
# reading a test configuration script that sets up the expected
# outcome of tests.

import re
import string
import time
from sys import exit
from os.path import abspath, join, basename, exists

SKIP = 'SKIP'
PASS = 'PASS'
FAIL = 'FAIL'
FAIL_OK = 'FAIL_OK'
CRASH = 'CRASH'
TIMEOUT = 'TIMEOUT'
SLOW = 'SLOW'

class Configuration(object):
  """The parsed contents of a configuration file"""
  def __init__(self, sections):
    self.sections = sections
  def get_rules(self, env):
    rules = { }
    for section in self.sections:
      if section.condition.evaluate(env):
        for rule in section.rules:
          rules[rule.name] = rule
    return Rules(env, rules)

class Rules(object):
  """A configuration specialized to a particular platform"""
  def __init__(self, env, rules):
    self.env = env
    self.rules = rules
  def __repr__(self):
    return "rules " + str(self.env) + " " + str(self.rules)
  def skip(self, name):
    if not name in self.rules:
      return False
    rule = self.rules[name]
    return rule.skip
  def cases(self):
    return self.rules.keys()
  def get_outcomes(self, name):
    """Returns a list of the allowable outcomes of the specified test"""
    if name in self.rules:
      return self.rules[name].get_outcomes(self.env)
    else:
      return [PASS]

class TestCase(object):
  def __init__(self, path, data, outcomes, spec):
    self.path = path
    self.outcomes = outcomes
    self.spec = spec
    self.data = data
  def expected_outcome(self, outcome):
    if outcome in self.outcomes:
      return True
    return (outcome == 'FAIL' and 'FAIL_OK' in self.outcomes)
  def start(self):
    self.spec.test_started(self)
  def done(self):
    self.spec.test_done(self)

STATISTICS_TEMPLATE = """\
%(count)4d tests scheduled to be run
 * %(skipped)4d tests will be skipped
 * %(nocrash)4d tests are expected to be flaky but not crash
 * %(pass)4d tests are expected to pass
 * %(fail_ok)4d tests are expected to fail that we won't fix
 * %(fail)4d tests are expected to fail that we should fix:\
"""

class TestSpecification(object):
  """A collection to which tests can be added.  Whenever a test is
  added it is checked against the rules for this test suite and only
  added if it is expected to be run."""
  def __init__(self, rules, skip_filter=None):
    self.rules = rules
    self.tests = { }
    self.skipped = [ ]
    self.skip_filter = skip_filter
    self.starts = { }
    self.times = { }

  def register_test(self, test, data=None, group="all"):
    outcomes = self.rules.get_outcomes(test)
    if (SKIP in outcomes) or (self.skip_filter and self.skip_filter(outcomes)):
      self.skipped.append(test)
    else:
      if not group in self.tests:
        self.tests[group] = [ ]
      self.tests[group].append(TestCase(test, data, outcomes, self))

  def test_started(self, test):
    self.starts[test.path] = time.time()

  def test_done(self, test):
    end = time.time()
    start = self.starts[test.path]
    del self.starts[test.path]
    diff = end - start
    if test.path in self.times:
      self.times[test.path] += diff
    else:
      self.times[test.path] = diff

  def cases(self, group="all"):
    if group in self.tests: return self.tests[group]
    else: return []

  def __str__(self):
    return "spec " + str(self.rules)

  def dump_statistics(self):
    def is_flaky(o):
      return (PASS in o) and (FAIL in o) and (not CRASH in o)
    all_tests = [ ]
    for group in self.tests.values():
      all_tests = all_tests + group
    failures = [t for t in all_tests if t.outcomes == [FAIL]]
    print STATISTICS_TEMPLATE % {
      'count': len(all_tests) + len(self.skipped),
      'skipped': len(self.skipped),
      'nocrash': len([t for t in all_tests if is_flaky(t.outcomes)]),
      'pass': len([t for t in all_tests if t.outcomes == [PASS]]),
      'fail_ok': len([t for t in all_tests if t.outcomes == [FAIL_OK]]),
      'fail': len(failures)
    }
    for failure in failures:
      print '        ' + basename(failure.path)
    print '---------------------'

  def dump_timing(self):
    items = self.times.items()
    sorted_items = [(v[1], v[0]) for v in items]
    sorted_items.sort()
    sorted_items.reverse()
    for i in xrange(0, min(20, len(sorted_items))):
        time = sorted_items[i][0]
        print '%(index)2d (%(time).3f s.): %(name)s' % {
            'index': i + 1,
            'time': time,
            'name': basename(sorted_items[i][1])
        }

class Section(object):
  """A section of the configuration file.  Sections are enabled or
  disabled prior to running the tests, based on their conditions"""
  def __init__(self, condition):
    self.condition = condition
    self.rules = [ ]
  def add_rule(self, rule):
    self.rules.append(rule)

class Rule(object):
  """A single rule that specifies the expected outcome for a single
  test."""
  def __init__(self, name, value):
    self.name = name
    self.value = value
  def __repr__(self):
    return str(self.value)
  def get_outcomes(self, env):
    set = self.value.get_outcomes(env)
    assert isinstance(set, ListSet)
    return set.elms

class Expression(object):
  pass

class Constant(Expression):
  def __init__(self, value):
    self.value = value
  def evaluate(self, env):
    return self.value
  def __repr__(self):
    return str(self.value)

class Variable(Expression):
  def __init__(self, name):
    self.name = name
  def evaluate(self, env):
    return self.name in env
  def get_outcomes(self, env):
    if self.evaluate(env): return Everything()
    else: return Nothing()
  def __repr__(self):
    return '$' + self.name

class Outcome(Expression):
  def __init__(self, name):
    self.name = name
  def __repr__(self):
    return self.name
  def get_outcomes(self, env):
    return ListSet([self.name])

class Operation(Expression):
  def __init__(self, left, op, right):
    self.left = left
    self.op = op
    self.right = right
  def evaluate(self, env):
    if self.op == '||':
      return self.left.evaluate(env) or self.right.evaluate(env)
    else:
      assert self.op == '&&'
      return self.left.evaluate(env) and self.right.evaluate(env)
  def get_outcomes(self, env):
    left = self.left.get_outcomes(env)
    right = self.right.get_outcomes(env)
    if self.op == '||':
      return left.union(right)
    else:
      assert self.op == '&&'
      return left.intersect(right)
  def __repr__(self):
    return "(" + str(self.left) + " " + self.op + " " + str(self.right) + ")"

# --- S e t s ---

class Set(object):
  pass

class ListSet(Set):
  def __init__(self, elms):
    self.elms = elms
  def intersect(self, that):
    if not isinstance(that, ListSet):
      return that.intersect(self)
    return ListSet([ x for x in self.elms if x in that.elms ])
  def union(self, that):
    if not isinstance(that, ListSet):
      return that.union(self)
    return ListSet(self.elms + [ x for x in that.elms if x not in self.elms ])

class Everything(Set):
  def intersect(self, that):
    return that
  def union(self, that):
    return self

class Nothing(Set):
  def intersect(self, that):
    return self
  def union(self, that):
    return that

# -------------------------------------------
# --- E x p r e s s i o n   P a r s i n g ---
# -------------------------------------------

def isalpha(str):
  for char in str:
    if not (char.isalpha() or char.isdigit() or char == '_'):
      return False
  return True

class Tokenizer(object):
  """A simple string tokenizer that chops expressions into variables,
  parens and operators"""

  def __init__(self, expr):
    self.index = 0
    self.expr = expr
    self.length = len(expr)
    self.tokens = None

  def current(self, length = 1):
    if not self.has_more(length): return ""
    return self.expr[self.index:self.index+length]

  def has_more(self, length = 1):
    return self.index < self.length + (length - 1)

  def advance(self, count = 1):
    self.index = self.index + count

  def add_token(self, token):
    self.tokens.append(token)

  def skip_spaces(self):
    while self.has_more() and self.current().isspace():
      self.advance()

  def tokenize(self):
    self.tokens = [ ]
    while self.has_more():
      self.skip_spaces()
      if not self.has_more():
        return None
      if self.current() == '(':
        self.add_token('(')
        self.advance()
      elif self.current() == ')':
        self.add_token(')')
        self.advance()
      elif self.current() == '$':
        self.add_token('$')
        self.advance()
      elif isalpha(self.current()):
        buf = ""
        while self.has_more() and isalpha(self.current()):
          buf += self.current()
          self.advance()
        self.add_token(buf)
      elif self.current(2) == '&&':
        self.add_token('&&')
        self.advance(2)
      elif self.current(2) == '||':
        self.add_token('||')
        self.advance(2)
      else:
        return None
    return self.tokens

OPERATORS = ["&&", "||"]

class Scanner(object):
  """A simple scanner that can serve out tokens from a given list"""

  def __init__(self, tokens):
    self.tokens = tokens
    self.length = len(tokens)
    self.index = 0

  def has_more(self):
    return self.index < self.length

  def current(self):
    return self.tokens[self.index]

  def advance(self):
    self.index = self.index + 1

def parse_atomic_expression(scan):
  if scan.current() == "true":
    scan.advance()
    return Constant(True)
  elif scan.current() == "false":
    scan.advance()
    return Constant(False)
  elif isalpha(scan.current()):
    name = scan.current()
    scan.advance()
    return Outcome(name)
  elif scan.current() == '$':
    scan.advance()
    if not isalpha(scan.current()):
      return None
    name = scan.current()
    scan.advance()
    return Variable(name)
  elif scan.current() == '(':
    scan.advance()
    result = parse_expression(scan)
    if (not result) or (scan.current() != ')'):
      return None
    scan.advance()
    return result
  else:
    return None

def parse_expression(scan):
  left = parse_atomic_expression(scan)
  if not left: return None
  while scan.has_more() and (scan.current() in OPERATORS):
    op = scan.current()
    scan.advance()
    right = parse_atomic_expression(scan)
    if not right:
      return None
    left = Operation(left, op, right)
  return left

def parse_logical_expression(expr):
  """Parses a logical expression into an Expression object"""
  tokens = Tokenizer(expr).tokenize()
  if not tokens:
    print "Malformed expression: '" + expr + "'"
    exit(1)
  scan = Scanner(tokens)
  ast = parse_expression(scan)
  if not ast:
    print "Malformed expression: '" + expr + "'"
    exit(1)
  return ast


# -------------------------------
# --- F i l e   R e a d i n g ---
# -------------------------------

HEADER_PATTERN = re.compile('\[([^]]+)\]')
RULE_PATTERN = re.compile('\s*(%?)([^= ]*)\s*=(.*)')

def read_configuration(path, root):
  """Reads the lines in a configuration file"""
  current_section = Section(Constant(True))
  sections = [ current_section ]
  for line in open(path):
    if '#' in line:
      line = line[:line.index('#')]
    line = line.strip()
    if not line: continue
    header_match = HEADER_PATTERN.match(line)
    if header_match:
      condition_str = header_match.group(1).strip()
      condition = parse_logical_expression(condition_str)
      new_section = Section(condition)
      sections.append(new_section)
      current_section = new_section
      continue
    rule_match = RULE_PATTERN.match(line)
    if rule_match:
      matches_file = (rule_match.group(1) != '%')
      name = rule_match.group(2).strip()
      if matches_file: name = abspath(join(root, name))
      if matches_file and (not exists(name)):
        print "There is no test '%s'." % name
        exit(1)
      value_str = rule_match.group(3).strip()
      value = parse_logical_expression(value_str)
      current_section.add_rule(Rule(name, value))
    else:
      print "Malformed line: '%s'." % line
      exit(1)
  return Configuration(sections)

if __name__ == '__main__':
  from sys import argv
  for f in argv[1:]:
    config = read_configuration(f)
    print config.get_rules(['NIRK'])
