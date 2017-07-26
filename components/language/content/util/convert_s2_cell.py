# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import csv
import os.path
import string
import sys

sys.path.insert(1,
    os.path.join(os.path.dirname(__file__), '../../../../third_party'))
import jinja2 # pylint: disable=F0401


ADMIN_DISTRICT_FIELD = "name"
LANG_FIELD = "languages"


program_name = os.path.basename(__file__)
if len(sys.argv) < 5 or sys.argv[1] != "-o":
    sys.stderr.write("Usage: %s -o OUTPUT_FILE INPUT_FILE\n" % program_name)
    exit(1)

output_path = sys.argv[2]
template_file_path = sys.argv[3]
data_file_path = sys.argv[4]

# Derive district : language_code pairs.
district_with_langs = []
with open(data_file_path, "r") as f:
    reader = csv.DictReader(f, delimiter=',')
    for row in reader:
        district_with_langs.append((row[ADMIN_DISTRICT_FIELD], row[LANG_FIELD]))

# Render the template to generate cpp code for LanguageCodeLocator.
generated_code = None
with open(template_file_path, "r") as f:
    template = jinja2.Template(f.read())
    context = {
        "district_with_langs" : district_with_langs
    }
    generated_code = template.render(context)

# Write the generated code.
with open(output_path, "w") as f:
    f.write(generated_code)
