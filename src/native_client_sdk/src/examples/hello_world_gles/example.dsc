{
  'TOOLS': ['newlib', 'glibc', 'pnacl'],
  'TARGETS': [
    {
      'NAME' : 'hello_world_gles',
      'TYPE' : 'main',
      'SOURCES' : ['hello_world.cc', 'matrix.cc', 'matrix.h'],
      'CXXFLAGS': [
        '$(NACL_CXXFLAGS)', 
        '-I../../src',
        '-I../../src/ppapi/lib/gl'
      ],
      'LDFLAGS': ['-g', '-pthread', '-lppapi', '-lppapi_gles2']
    }
  ],
  'DATA': [
    'fragment_shader_es2.frag',
    'hello.raw',
    'vertex_shader_es2.vert'
  ],
  'DEST': 'examples',
  'NAME': 'hello_world_gles',
  'TITLE': 'Hello World GLES 2.0',
  'DESC': """
The Hello World GLES 2.0 example demonstrates how to create a 3D cube
that rotates.  This is a simpler example than the tumbler example, and 
written in C.  It loads the assets using URLLoader.""",
  'INFO': 'Teaching focus: 3D graphics, URL Loader.'
}


