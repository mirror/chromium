import sys, os 

if __name__ == '__main__':
  srcdir = sys.argv[1]
  tgtdir = sys.argv[2]

  if not os.path.exists(tgtdir):
    os.mkdir(tgtdir)

  js_source = os.path.join(srcdir, 'SOURCE-js')
  natives = []
  f = open(js_source)
  try:
    for line in f:
      natives.append(os.path.join(srcdir, line.strip()))
  finally:
    f.close()
 
  toolsdir = os.path.join(srcdir, '..', 'tools')

  sys.path.append(toolsdir)
  import js2c

  natives_cc = os.path.join(tgtdir, 'natives.cc')
  natives_light_cc = os.path.join(tgtdir, 'natives_light.cc')
  
  js2c.JS2C(natives, [ natives_cc, natives_light_cc ], env=None)
  js2c.MakeVersion(os.path.join(srcdir, '..', 'version.ini'), os.path.join(tgtdir, 'version.cc'))
