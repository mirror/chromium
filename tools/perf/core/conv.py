import sys
import json

with open(sys.argv[1]) as f:
  d = f.readlines()

d.pop(0)
data = {}
for line in d:
  a, b = line.split(',')
  if 'reference' in a:
    continue
  data[a] = float(b.strip())

with open(sys.argv[2], 'w') as out:
  json.dump(data, out, indent=4, separators=(',', ': '), sort_keys=True)
