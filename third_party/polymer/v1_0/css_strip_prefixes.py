import glob
import re

# List of CSS properties to be removed.
css_properties_to_remove = [
  '-moz-appearance',
  '-moz-box-sizing',
  '-moz-flex-basis',
  '-moz-user-select',

  '-ms-align-content',
  '-ms-align-self',
  '-ms-flex',
  '-ms-flex-align',
  '-ms-flex-basis',
  '-ms-flexbox',
  '-ms-flex-direction',
  '-ms-flex-pack',
  '-ms-flex-wrap',
  '-ms-inline-flexbox',
  '-ms-user-select',

  '-webkit-align-content',
  '-webkit-align-items',
  '-webkit-align-self',
  '-webkit-animation',
  '-webkit-animation-duration',
  '-webkit-animation-iteration-count',
  '-webkit-animation-name',
  '-webkit-animation-timing-function',
  '-webkit-flex',
  '-webkit-flex-direction',
  '-webkit-flex-wrap',
  '-webkit-inline-flex',
  '-webkit-justify-content',
  '-webkit-transform',
  '-webkit-transform-origin',
  '-webkit-transition',
  '-webkit-transition-delay',
  '-webkit-transition-property',
  '-webkit-user-select',
]


def processFile(filename):
  print 'processing ' + filename

  # Gather indices of lines to be removed.
  indicesToRemove = [];
  with open(filename) as f:
    lines = f.readlines()
    for i, line in enumerate(lines):
      if any(re.search(p, line) for p in css_properties_to_remove):
        indicesToRemove.append(i)

  # Process line numbers in descinding order, such that the array can be
  # modified in-place.
  indicesToRemove.reverse()
  for i in indicesToRemove:
    del lines[i]

  # Reconstruct file.
  with open(filename, 'w') as f:
    for l in linesAfter:
      f.write(l)
  return


def main():
  filenames = glob.glob('components-chromium/**/*.html')
  for f in filenames:
    processFile(f)


if __name__ == '__main__':
  main()
