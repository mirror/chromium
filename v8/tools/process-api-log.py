#!/usr/bin/python

# Processes a v8 log and extracts api performance information, and
#outputs the results formatted as twiki tables.

import sys, re, string

def print_map_sorted(map, limit):
  values = map.values()
  values.sort(reverse=True)
  for value in values:
    if value < limit: break
    for (k, v) in map.items():
      if v is value:
        print '| ' + k + ' | ' + str(v) + ' |'
        del map[k]
        break
  print ""

def print_function_hits(limit):
  PATTERN = re.compile("\\(api (.*)\\)")
  hits = { }
  for line in file(sys.argv[1]):
    match = PATTERN.match(line)
    if match:
      body = map(string.strip, match.group(1).split())
      function = body[0]
      if not function in hits:
        hits[function] = 0
      hits[function] += 1
  print '| *Operation* | *count* |'
  print_map_sorted(hits, limit)

def print_security_hits(limit):
  PATTERN = re.compile("\\(api check-security (.*)\\)")
  hits = { }
  for line in file(sys.argv[1]):
    match = PATTERN.match(line)
    if match:
      name = match.group(1)
      if not name in hits:
        hits[name] = 0
      hits[name] += 1
  print '| *Name* | *count* |'
  print_map_sorted(hits, limit)

def print_load_hits(limit):
  PATTERN = re.compile("\\(api load \"([^ ]+)\" ([^ ]+)\\)")
  hits = { }
  for line in file(sys.argv[1]):
    match = PATTERN.match(line)
    if match:
      entry = match.group(1) + '[' + match.group(2) + ']'
      if not entry in hits:
        hits[entry] = 0
      hits[entry] += 1
  print '| *Name* | *count* |'
  print_map_sorted(hits, limit)


print "---+++ Operations\n"
print "Truncated at 100 hits\n"

print_function_hits(100)

print "---+++ Security checks\n"
print "Truncated at 80 hits\n"

print_security_hits(80)

print "---+++ Accessor loads\n"
print "Truncated at 20 hits\n"

print_load_hits(20)
