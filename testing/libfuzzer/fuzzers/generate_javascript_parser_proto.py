#!/usr/bin/python3

# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

def parse_word(word_string):
  # Every part of the word is either a string surrounded by "" or a placeholder
  # $<int>.
  print(word_string)

  parts = []
  while len(word_string) > 0:
    if word_string[0] == '"':
      end_ix = 1 + word_string[1:].index('"')
      parts.append(word_string[1:end_ix])
      word_string = word_string[(end_ix + 1):]
    elif word_string[0] == '$':
      end_ix = word_string.index(' ')
      parts.append(int(word_string[1:end_ix]))
      word_string = word_string[end_ix:]
    else:
      assert(False)
    word_string = word_string.lstrip()
  return parts

def generate_proto_contents(d):
  c = ''
  ix = 0
  for word in d:
    c = c + '    token_value_' + str(ix) + ' = ' + str(ix) + ';\n'
    ix += 1
  return c

def read_dictionary(filename):
  with open(filename) as f:
    lines = f.readlines()
  words = []
  for line in lines:
    if not line.startswith('#'):
      words.append(parse_word(line))
  return words

if __name__ == '__main__':
  d = read_dictionary('dicts/javascript_parser_proto.dict')

  proto_header = """
syntax = "proto2";

package javascript_parser_proto_fuzzer;

message Token {
  enum Value {
"""


  proto_footer = """  }
  required Value value = 1;
  repeated Token inner_tokens = 2;
}

message Source {
  repeated Token tokens = 1;
}
"""

  proto_contents = generate_proto_contents(d)

  print(proto_header + proto_contents + proto_footer)


  header = """
std::string token_to_string(const javascript_parser_proto_fuzzer::Token& token) {
  switch(token.value()) {""";
