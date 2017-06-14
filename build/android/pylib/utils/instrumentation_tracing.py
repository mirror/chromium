# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""[DOCUMENTATION]"""

import inspect
import json
import os
import sys
import threading

from py_trace_event import trace_event

class _TraceArguments(object):
  """Wraps a dictionary but ensure safe evaluation of repr()."""

  def __init__(self):
    self.internal = {}

  def add(self, key, val):
    self.internal[key] = val

  def __repr__(self):
    s = "{"

    for key in self.internal:
      try:
        s += repr(key)
      except (AttributeError, AssertionError):
        try:
          s += str(key)
        except (AttributeError, AssertionError):
          s += "<ERROR>"

      s += ": "

      val = self.internal[key]
      try:
        s += repr(val)
      except (AttributeError, AssertionError):
        try:
          s += str(val)
        except (AttributeError, AssertionError):
          s += "<ERROR>"

      s += ", "

    if len(s):
      s = s[:-2]
    s += "}"

    return s


saved_thread_ids = set()

def _traceFunction(frame, event, arg, to_include, to_exclude, output_file):
  to_exclude = set(to_exclude)
  to_exclude.add("py_trace_event")

  if event == "call" or event == "return":
    function_name = frame.f_code.co_name
    filename = frame.f_code.co_filename

    # print("RUNNING %s FROM %s" % (function_name, filename))

    if any([item in filename for item in to_include]) and \
        not any([item in filename for item in to_exclude]):
      outer_frames = inspect.getouterframes(frame)
      all_filenames = [fr[0].f_code.co_filename for fr in outer_frames]

      for this_filename in all_filenames:
        for item in to_exclude:
          if item in this_filename:
            return

      if event == "call":
        # starting function: save thread if not already done; record Begin
        # event; return this function (to be used as local trace function)
        thread_id = threading.current_thread().ident
        if thread_id not in saved_thread_ids:
          thread_name = threading.current_thread().name
          process_id = os.getpid()
          metadata = {"pid": process_id, "tid": thread_id, "ts": 0, "ph": "M",
                      "cat": "__metadata", "name": "thread_name",
                      "args": {"name": thread_name}}
          metadata_str = json.dumps(metadata)
          handle = open(output_file, "a")
          handle.write(", " + metadata_str)
          handle.close()
          saved_thread_ids.add(thread_id)

        line_number = frame.f_lineno
        if frame.f_code.co_argcount > 0:
          arguments = _TraceArguments()
          for idx in range(frame.f_code.co_argcount):
            arg_name = frame.f_code.co_varnames[idx]
            arguments.add(arg_name, frame.f_locals[arg_name])
          trace_event.trace_begin(function_name, arguments=arguments,
                                  filename=filename, line_number=line_number)
        else:
          trace_event.trace_begin(function_name, filename=filename,
                                  line_number=line_number)
        traceFunc = lambda frame, event, arg: _traceFunction(frame, event, arg,
                                                             to_include,
                                                             to_exclude,
                                                             output_file)
        return traceFunc
      else:
        trace_event.trace_end(function_name)

def StartInstrumenting(to_include, to_exclude, output_file):
  trace_event.trace_enable(output_file)

  traceFunc = lambda frame, event, arg: _traceFunction(frame, event, arg,
                                                       to_include, to_exclude,
                                                       output_file)

  sys.settrace(traceFunc)
  threading.settrace(traceFunc)

