# Copyright 2006-2008 Google Inc. All Rights Reserved.


# Testing framework for V8 based on SCons.
#
# The testing framework extracts individual tests from .cc files
# by using a regular expression that looks for (static) functions
# of the form Test..., e.g. TestFoo and TestBarBaz.
#
# The framework compiles each given .cc file into one parameterized
# executable that takes the name of a test as a string and executes
# the given test, e.g.
#
#   ./.mytest.cc.run TestMyFunction
#
# The framework can run the individual tests and report if the
# the tests were succesful or not.

import sys, os, select, difflib, platform, time, unittest, re, tempfile, signal
import subprocess
import platform_utils
import testconfig
from os import path
from os.path import join, abspath, basename

# Constants for test status.
SKIP = 0
PASS = 1
FAIL = 2
FLAKY = 3
SLOW = 4
FAIL_OK = 5
STATUS_MAP = { 'skip': SKIP, 'pass': PASS, 'fail': FAIL,
               'flaky': FLAKY, 'slow': SLOW, 'fail_ok': FAIL_OK }

def ReadListEntries(name):
  result = [ ]
  for line in open(name):
    if '#' in line:
      line = line[:line.index('#')]
    line = line.strip()
    if len(line) > 0:
      result.append(line)
  return result

def ReadList(name, prefix):
    test_to_status = {}
    status_to_tests = {}
    for s in STATUS_MAP.itervalues(): status_to_tests[s] = []
    entries = ReadListEntries(name)
    for line in entries:
        split = line.index('=')
        status = STATUS_MAP[line[split+1:].strip().lower()]
        name = line[:split].strip()
        test = abspath(join(prefix, name))
        if not os.path.isfile(test):
            raise SyntaxError('Non-existing test file: ' + test)
        test_to_status[test] = status
        status_to_tests[status].append(test)
    return (test_to_status, status_to_tests)


# Each file under test generates two files: A compilable file
# that defines a suitable main() function and an executable
# file that allows the tests to run. The names of the files
# are controlled by the following two functions:
def GetCompilable(env, path):
    return env['OBJPREFIX'] + os.path.basename(path) + '.main.cc'

def GetExecutable(env, path):
    return env['OBJPREFIX'] + os.path.basename(path) + '.run' + env['PROGSUFFIX']

all_tests = unittest.TestSuite()

def GetSuite(enviro):
    if enviro['is_common']: return all_tests
    try:
        suite = enviro.suite
    except:
        suite = unittest.TestSuite()
        enviro.suite = suite
    return suite

# Add more signals as necessary
SIG_MAP = {
  8: 'SIGFPE', 4: 'SIGILL', 9: 'SIGKILL', 3: 'SIGQUIT', 11: 'SIGSEGV',
  5: 'SIGTERM'
}

# Helper function to run a given command, returning the output
# (stdout, stderr) as a tuple.
def RunCommand(command, ignore_stderr, is_negative, timeout):
    def IsExpectedExitCode(cause, detail, timed_out):
        aborted = (cause == 0) and (((detail & 0x7f) == signal.SIGABRT))
        if timed_out:
            return False
        elif (cause == 0) and (detail & 0x80) and (not aborted):
            return aborted
        elif is_negative:
            return (cause != 0) or (detail != 0)
        else:
            return (cause == 0) and (detail == 0)

    def FormatExitCodeError(cause, detail, timed_out):
        aborted = (cause == 0) and (((detail & 0x7f) == signal.SIGABRT))
        if timed_out:
            return 'Failure: Timed out\n';
        elif (cause == 0) and (detail & 0x80) and (not aborted):
            sig = detail & 0x7f
            if sig in SIG_MAP: name = SIG_MAP[sig]
            else: name = str(sig)
            return 'Failure: Crash (signal: ' + name + ')\n'
        elif is_negative:
          return 'Failure: Negative test succeeded. Expected non-zero exit code.\n'
        else:
          return 'Failure: Non-zero exit code (' + str(detail) + ')\n'

    # Now that we are using the subprocess module, we are requiring
    # Python 2.4 or greater.

    # Generate temporary files for the output.
    (fd_out, outname) = tempfile.mkstemp()
    (fd_err, errname) = tempfile.mkstemp()
    # Run the command and get the exit code.
    end_time = time.time() + timeout
    command = command.split(' ')
    process = subprocess.Popen(command, stdout=fd_out, stderr=fd_err)
    code = None
    timed_out = False
    while code == None:
        if (timeout >= 0) and (time.time() >= end_time):
            platform_utils.Kill(process.pid)
            code = process.wait()
            timed_out = True
        else:
            code = process.poll()
            time.sleep(0.02)
    # Close the temporary files.
    os.close(fd_out); os.close(fd_err)
    # Exit code stuff.
    cause = code & 0xff
    detail = code >> 8
    if cause == signal.SIGINT: signal.default_int_handler()
    # Get the output and the error output (maybe ignored).
    out = file(outname).read()
    if ignore_stderr: err = ''
    else: err = file(errname).read()
    # Unlink the temporary files.
    os.unlink(outname); os.unlink(errname)
    # Check the exit code and return the output pair.
    if not IsExpectedExitCode(cause, detail, timed_out):
        err += FormatExitCodeError(cause, detail, timed_out)
    return (out, err)


# Read the golden file (.out or .err); return empty string if file not
# found.
def ReadGolden(script, extension):
    path = script[:-3] + extension
    if (not os.path.exists(path)): return ''
    return file(path).read()


# Helper function to compute the difference between two strings.
# TODO(kasperl): Make sure we do not allow missing linefeeds at the
# end of the strings.
def ComputeDiff(s1, s2):
    return ''.join(difflib.ndiff(s2.splitlines(1), s1.splitlines(1)))


# -----------------------------------------------------------------------------

executables = []

class CTestCase(unittest.TestCase):
    def __init__(self, case, executable, test, flags, variant, timeout):
        super(CTestCase, self).__init__()
        self.case = case
        self.executable = executable
        self.test = test
        self.flags = flags
        self.variant = variant
        self.timeout = timeout

    def __str__(self):
        return self.variant + ' ' + self.case.path + ':Test' + self.test + '()'

    def __repr__(self):
        return '<' + str(self) + '>'

    def ComputeCommand(self):
        result = [os.path.abspath(self.executable), self.test]
        result += self.flags
        return ' '.join(result)

    # Runs the test corresponding to this test case and captures
    # the output. If the test fails, the output is used as the
    # failure message.
    def runTest(self):
        command = self.ComputeCommand()
        self.case.start()
        (out, err) = RunCommand(command, True, False, self.timeout)
        self.case.done()
        if (err == ''): outcome = testconfig.PASS
        else: outcome = testconfig.FAIL
        # Because of the way these tests are set up it is much easier
        # to permit tests to unexpectedly pass.  So we do.
        if self.case.expected_outcome(outcome) or outcome == testconfig.PASS:
            return
        if len(err) > 0: print err
        if len(out) > 0: print out
        self.fail(out)


class JSTestCase(unittest.TestCase):
    def __init__(self, program, arguments, case, variant, is_negative,
                 timeout):
        super(JSTestCase, self).__init__()
        self.program = program
        self.arguments = arguments
        self.case = case
        self.command = self.ComputeCommand()
        self.variant = variant
        self.is_negative = is_negative
        self.timeout = timeout

    def __str__(self):
        return self.variant + ' ' + path.basename(self.case.path)

    def __repr__(self):
        return '<' + str(self) + '>'

    def ComputeCommand(self):
	result = [ os.path.abspath(self.program) ]
	result.extend(self.arguments)
	result.append(self.case.path)
	return ' '.join(result)

    # Runs the JavaScript test and compares the output with the
    # golden output from the .out and .err files.
    def runTest(self):
        self.case.start()
        (out, err) = RunCommand(self.command, False, self.is_negative,
                                self.timeout)
        self.case.done()
        out = out.replace('\r\n', '\n')
        err = err.replace('\r\n', '\n')
        goldenOut = ReadGolden(self.case.path, '.out')
        goldenErr = ReadGolden(self.case.path, '.err')
        goldenOut = goldenOut.replace('\r\n', '\n')
        goldenErr = goldenErr.replace('\r\n', '\n')
        okayOut = (goldenOut == out)
        okayErr = (goldenErr == err)
        if (okayOut and okayErr): outcome = testconfig.PASS
        else: outcome = testconfig.FAIL
        if self.case.expected_outcome(outcome): return
        # Compute failure message and fail.
        if outcome == testconfig.FAIL:
          message = ''
          if (not okayOut): message += ("\n==== Unexpected output ====\n" + ComputeDiff(out, goldenOut))
          if (not okayErr): message += ("\n==== Unexpected errors ====\n" + ComputeDiff(err, goldenErr))
        else:
          message = 'Test was expected to fail but passed.'
        self.fail(message)


class MiniJSUnitTestCase(unittest.TestCase):
  def __init__(self, program, arguments, case, mjsunitScript, variant,
               timeout):
    super(MiniJSUnitTestCase, self).__init__()
    self.program = program
    self.arguments = arguments
    self.case = case
    self.mjsunitScript = mjsunitScript
    self.command = self.ComputeCommand()
    self.variant = variant
    self.timeout = timeout

  def __str__(self):
      return self.variant + ' ' + path.basename(self.case.path)

  def __repr__(self):
      return '<' + str(self) + '>'

  def ComputeCommand(self):
      result = [ os.path.abspath(self.program) ]
      result.extend(self.arguments)
      # If the first line of the test case JS file contains
      # extra arguments for the script invocation, we add them
      # to the command right after the standard arguments.
      first_line = file(self.case.path).readline()
      if first_line.startswith('// Flags:'):
          result.extend(first_line[9:].rstrip().split())
      # Append the mjsunit.js and the test case JS file.
      result.append(self.mjsunitScript)
      result.append(self.case.path)
      return ' '.join(result)

  # Runs the JavaScript test and processes the output
  def runTest(self):
    self.case.start()
    (out, err) = RunCommand(self.command, False, False, self.timeout)
    self.case.done()
    okayErr = (err == '')
    failureLines = []
    for outLine in out.split('\n'):
      if ("Failure" in outLine) or ("Uncaught" in outLine):
        failureLines.append(outLine)
    okayOut = (len(failureLines) == 0)
    if okayOut and okayErr: outcome = testconfig.PASS
    else: outcome = testconfig.FAIL
    if self.case.expected_outcome(outcome): return
    # Compute failure message and fail.
    if outcome == testconfig.FAIL:
      message = ''
      if (not okayOut):
        message += ("\n==== Unexpected output ====\n")
        for failure in failureLines:
          message += failure + "\n"
      if (not okayErr): message += ("\n==== Unexpected errors ====\n" +  err)
    else:
      message = 'Test was expected to fail but passed.'
    self.fail(message)


class CustomTestCase(unittest.TestCase):
  def __init__(self, name, function):
    super(CustomTestCase, self).__init__()
    self.function = function
    self.name = name

  def __str__(self):
    return self.name

  def __repr__(self):
    return '<' + str(self) + '>'

  def runTest(self):
    self.function(self)


# Mozilla tests fail when the output contains 'failed!' in any
# combination of lower- and uppercase letters.
mozillaFailureMatcher = re.compile(r'failed!', re.IGNORECASE)


def MozillaPrintName(script):
    # Include two levels of directories in the printed name.
    (prefix, filename) = path.split(script)
    (prefix, dir2) = path.split(prefix)
    (prefix, dir1) = path.split(prefix)
    return path.join(dir1, dir2, filename)

  
class MozillaTestCase(unittest.TestCase):
  def __init__(self, program, arguments, script, variant, case, timeout):
    super(MozillaTestCase, self).__init__()
    self.program = program
    self.arguments = arguments
    self.script = script
    self.command = self.ComputeCommand()
    self.variant = variant
    self.case = case
    self.timeout = timeout

  def __str__(self):
      return self.variant + ' mozilla ' + MozillaPrintName(self.script)

  def __repr__(self):
      return '<' + str(self) + '>'

  def ComputeCommand(self):
    result = [ os.path.abspath(self.program) ]
    result.extend(self.arguments)
    result.append(self.script)
    return ' '.join(result)

  # Runs the JavaScript test and processes the output
  def runTest(self):
    is_negative = self.script.endswith('-n.js')
    # Run the test.
    self.case.start()
    (out, err) = RunCommand(self.command, False, is_negative, self.timeout)
    self.case.done()
    # TODO(kasperl): Check for crashes - always.
    failed = (err != '') or (not mozillaFailureMatcher.search(out) is None)
    if failed: outcome = testconfig.FAIL
    else: outcome = testconfig.PASS
    if not self.case.expected_outcome(outcome):
        message = "\n--- Test output ---\n" + out
        if (err != ''): message += "\n--- Test errors ---\n" + err
        if outcome == testconfig.PASS:
            self.fail('The test case unexpectedly passed.' + message)
        else:
            assert outcome == testconfig.FAIL
            self.fail('The test case unexpectedly failed.' + message)


class SpecialTestCase(unittest.TestCase):

  def __init__(self, program, test, command, variant, timeout):
    super(SpecialTestCase, self).__init__()
    self.program = program
    self.command = command
    self.test = test
    self.variant = variant
    self.timeout = timeout

  def ComputeCommand(self):
    exe = str(self.program) + " " + self.test.path
    return self.command % { 'VM': exe  }
 
  def __str__(self):
    return self.variant + ' special ' + basename(self.test.path)

  def runTest(self):
    self.test.start()
    (out, err) = RunCommand(self.ComputeCommand(), False, False, self.timeout)
    self.test.done()
    failed = (err != '')
    if failed: outcome = testconfig.FAIL
    else: outcome = testconfig.PASS
    if outcome != testconfig.PASS and not self.test.expected_outcome(outcome):
      message = "\n--- Test output ---\n" + out
      if (err != ''): message += "\n--- Test errors ---\n" + err
      self.fail('The test case unexpectedly failed.' + message)


class EsctfTestCase(unittest.TestCase):
  def __init__(self, program, arguments, test, variant, framework):
    super(EsctfTestCase, self).__init__()
    self.program = program
    self.arguments = arguments
    self.test = test
    self.variant = variant
    self.framework = framework
    self.command = self.ComputeCommand()

  def __str__(self):
    return self.variant + ' esctf ' + basename(self.test.path);

  def ComputeCommand(self):
    result = [ os.path.abspath(self.program) ]
    result.append(self.framework)
    result.extend(self.arguments)
    result.append(self.test.path)
    return ' '.join(result)

  def runTest(self):
    f = open(self.test.path)
    contents = f.read()
    f.close()
    negative = ("@negative" in contents)
    self.test.start()
    (out, err) = RunCommand(self.command, False, False, -1)
    self.test.done()
    failed = ("Error" in out) or ("Fail" in out) or err
    if negative: failed = not failed
    if failed: outcome = testconfig.FAIL
    else: outcome = testconfig.PASS
    if self.test.expected_outcome(outcome): return
    if outcome == testconfig.FAIL:
      message = "\n--- Output ---\n" + out + "\n--- Error ---\n" + err;
      self.fail(message)
    else:
      self.fail("Test unexpectedly passed")


scanCache = {}
scanRegExp = r'^\s*(?:static)?\s*void\s+Test(\w+)\s*\(\s*\)\s*{\s*$'
scanMatcher = re.compile(scanRegExp, re.M)

def ScanTests(node):
    # Canonicalize the path before checking the cache.
    path = node.rfile().abspath
    if path in scanCache: return scanCache[path]
    contents = file(path).read()
    result = scanMatcher.findall(contents)
    scanCache[path] = result
    return result

def BuildCompilable(env, target, source):
    f = open(str(target[0]), 'w')
    tests = ScanTests(source[0])
    f.write('const int kNumberOfTests = ' + `len(tests)` + ';\n')
    f.write('#include "' + source[0].abspath + '"\n')
    f.write('#include <string.h>\n')
    f.write('#include <stdlib.h>\n')
    f.write('int main(int argc, char** argv) {\n');
    f.write('  v8::internal::FlagList::SetFlagsFromCommandLine(&argc, argv, true);\n')
    f.write('  if (argc == 1) {\n')
    f.write('    printf("========= Possible tests are =========\\n");\n')
    first = True
    for test in tests:
      f.write('    printf("')
      if first: first = False
      else: f.write(" ")
      f.write(test + '");\n')
    f.write('    printf("\\n");\n')
    f.write('    exit(1);\n')
    f.write('  }\n')
    f.write('  bool ran_test = false;\n')
    f.write('  for (int i = 1; i < argc; i++) {\n')
    contains_threading = 0;
    for test in tests:
      f.write('    if (strcmp(argv[i], "' + test + '") == 0) {\n')
      f.write('      ran_test = true;\n')
      f.write('      Test' + test + '();\n')
      f.write('    }\n')
      if test == 'Threading':
        contains_threading = 1;
    f.write('  }\n')
    f.write('  if (!ran_test) {\n')
    f.write('    printf("Warning: no tests were run.\\n");\n')
    f.write('  }\n');
    f.write('}\n');
    if contains_threading:
      f.write('bool ExcludedTest(char* name);\n');
      f.write('void CallTestNumber(int test_number_) {\n');
      f.write('  switch (test_number_) {\n');
      i = 0;
      for test in tests:
        f.write('    case ' + `i` + ': if (!ExcludedTest("' + test + '")) Test' + test + '(); break;\n');
        i += 1;
      f.write('    default: break;\n');
      f.write('  }\n');
      f.write('}\n');
    f.close()

def AddTests(env, case, flags, sources):
    subsuite = unittest.TestSuite()
    compilable = GetCompilable(env, case.path)
    executable = GetExecutable(env, case.path)
    env.Command(compilable, case.path, env.Action(BuildCompilable, None))
    program = env.Program(executable, [compilable] + sources)
    executables.append(program)
    for test in ScanTests(env.File(case.path)):
        subsuite.addTest(CTestCase(case, abspath(executable), test,
                                   flags, env['variant'], env['timeout']))
    GetSuite(env).addTest(subsuite)
    all_tests.addTest(subsuite)

def AddScript(env, tester, script, is_negative):
    subsuite = unittest.TestSuite()
    path = tester[0][0]
    if (path not in executables): executables.append(path)
    subsuite.addTest(JSTestCase(path.abspath, tester[1:], script,
                                env['variant'], is_negative, env['timeout']))
    GetSuite(env).addTest(subsuite)
    all_tests.addTest(subsuite)

def AddMiniJSUnitTest(env, tester, case, mjsunitScript):
    subsuite = unittest.TestSuite()
    path = tester[0][0]
    if (path not in executables): executables.append(path)
    subsuite.addTest(MiniJSUnitTestCase(path.abspath, tester[1:], case,
                                        mjsunitScript, env['variant'],
                                        env['timeout']))
    GetSuite(env).addTest(subsuite)
    all_tests.addTest(subsuite)

def AddMozillaTest(env, tester, case):
    subsuite = unittest.TestSuite()
    path = tester[0][0]
    if (path not in executables): executables.append(path)
    subsuite.addTest(MozillaTestCase(path.abspath, tester[1:], case.path,
                                     env['variant'], case,
                                     env['timeout']))
    GetSuite(env).addTest(subsuite)
    all_tests.addTest(subsuite)

def AddSpecialTest(env, program, test, command):
    subsuite = unittest.TestSuite()
    path = program[0]
    if (path not in executables): executables.append(path)
    subsuite.addTest(SpecialTestCase(path, test, command, env['variant'], 
                                     env['timeout']))
    GetSuite(env).addTest(subsuite)
    all_tests.addTest(subsuite)

def AddEsctfTest(env, tester, test, esctf_framework):
    subsuite = unittest.TestSuite()
    path = tester[0][0]
    if (path not in executables): executables.append(path)
    subsuite.addTest(EsctfTestCase(path.abspath, [], test,
                                   env['variant'], esctf_framework))
    GetSuite(env).addTest(subsuite)
    all_tests.addTest(subsuite)

def AddCustomTest(env, name, function):
    subsuite = unittest.TestSuite()
    subsuite.addTest(CustomTestCase(name, function))
    GetSuite(env).addTest(subsuite)
    all_tests.addTest(subsuite)

from unittest import TestResult

class CompactTestResult(TestResult):
    line_length = 79
    separator1 = '=' * line_length
    separator2 = '-' * line_length
    status_line = "[%(min)02d:%(sec)02d|%(t) 4i%%|P%(s) 4d|F%(f) 4d] %(name)s"

    def __init__(self, stream, start_time, test_count):
        TestResult.__init__(self)
        self.stream = stream
        self.success_count = 0
        self.failure_count = 0
        self.start_time = start_time
        self.test_count = test_count

    def getDescription(self, test):
        return str(test)

    def startTest(self, test):
        TestResult.startTest(self, test)
        self.updateStatus(self.getDescription(test))

    def updateStatus(self, message):
        current_time = time.time()
        elapsed = current_time - self.start_time
        tests_run = self.success_count + self.failure_count
        percent = 0
        if self.test_count > 0:
            percent = 100 * float(tests_run) / self.test_count
        line = CompactTestResult.status_line % {
            'min': int(elapsed) / 60,
            'sec': int(elapsed) % 60,
            's': self.success_count,
            'f': self.failure_count,
            't': percent,
            'name': self.truncate(message)
        }
        self.stream.write(self.clearLine() + line)

    def truncate(self, line):
        max = (CompactTestResult.line_length - 27)
        if len(line) > max:
            return "..." + line[-(max-3):]
        else:
            return line

    def done(self):
        if self.failure_count == 0:
            self.updateStatus(self.statusMessage(True))
            return 0
        plural = 's'
        if self.failure_count == 1: plural = ''
        message = self.statusMessage(False) % {
            'count': self.failure_count,
            'plural': plural
        }
        self.updateStatus(message)
        return self.failure_count

    def addSuccess(self, test):
        TestResult.addSuccess(self, test)
        self.success_count = self.success_count + 1

    def addError(self, test, err):
        TestResult.addError(self, test, err)
        self.failure_count = self.failure_count + 1

    def addFailure(self, test, err):
        TestResult.addFailure(self, test, err)
        [type, value, traceback] = err
        self.stream.write(self.clearLine())
        self.stream.write("=== Failure in " + str(test) + " ===\n")
        self.stream.write(str(value).strip())
        self.stream.write("\n")
        self.failure_count = self.failure_count + 1

    def printErrors(self):
        self.printErrorList('ERROR', self.errors)
        self.printErrorList('FAIL', self.failures)

    def printErrorList(self, flavour, errors):
        for test, err in errors:
            self.stream.writeln(self.separator1)
            self.stream.writeln("%s: %s" % (flavour,self.getDescription(test)))
            self.stream.writeln(self.separator2)
            self.stream.writeln("%s" % err)

class PosixCompactTestResult(CompactTestResult):
    clear_line = "\r\033[2K"
    passed_status_message = "\033[32mAll tests passed.\033[0m\n"
    failed_status_message = "\033[31m%(count)i test%(plural)s failed.\033[0m\n"

    def __init__(self, stream, start_time, test_count):
        CompactTestResult.__init__(self, stream, start_time, test_count)
    
    def clearLine(self):
        return PosixCompactTestResult.clear_line

    def statusMessage(self, passed):
        if passed: return PosixCompactTestResult.passed_status_message
        else: return PosixCompactTestResult.failed_status_message

class WindowsCompactTestResult(CompactTestResult):
    clear_line = "\r" + (" " * CompactTestResult.line_length) + "\r"
    passed_status_message = "All tests passed.\n"
    failed_status_message = "%(count)i test%(plural)s failed.\n"

    def __init__(self, stream, start_time, test_count):
        CompactTestResult.__init__(self, stream, start_time, test_count)
    
    def clearLine(self):
        return WindowsCompactTestResult.clear_line
    
    def statusMessage(self, passed):
        if passed: return WindowsCompactTestResult.passed_status_message
        else: return WindowsCompactTestResult.failed_status_message

def RunTestSuite(suite, os):
    start_time = time.time()
    if os == 'win32':
        result = WindowsCompactTestResult(sys.stderr, start_time, suite.countTestCases())
    else:
        result = PosixCompactTestResult(sys.stderr, start_time, suite.countTestCases())
    suite.run(result)
    return result.done()


# Run the tests and return the number of failures.
def DoRun(env, target, source):
    if env['CROSS_COMPILING']:
        print "Not running cross-compiled tests"
        return 0

    output = env['output']
    if output == 'fancy':
        result =  RunTestSuite(GetSuite(env), env.os)
    else:
        # Compute the verbosity for the test runner based on the
        # requested output mode.
        verbosity = { 'simple': 2, 'bot': 1 }[output]
        result = unittest.TextTestRunner(verbosity=verbosity).run(GetSuite(env))
        # We don't have the count of failures, so return 0|1 so that
        # automation can detect whether the tests failed or not.
        result = not result.wasSuccessful()
    if env['time'] == 'yes' and 'test_spec' in env.Dictionary():
        print "--- Top 20 slowest tests ---"
        env['test_spec'].dump_timing()
    return result

def RunTests(env):
    dummy = 'testing'
    if env['rebuild'] == 'yes': dependencies = executables
    else: dependencies = []
    env.Command(dummy, dependencies, env.Action(DoRun, None))
    env.AlwaysBuild(dummy)
    return dummy
