import argparse
import sys

from util import build_utils


if __name__ == '__main__':
  args = build_utils.ExpandFileArgs(sys.argv[1:])
  parser = argparse.ArgumentParser()
  parser.add_argument('filearg')
  parser.add_argument('output')
  args = parser.parse_args(args)
  with open(args.output, 'w') as f:
    f.writelines('\n'.join(build_utils.ParseGnList(args.filearg)))
