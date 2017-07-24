setup({explicit_done : true});

function testParseStyle() {
  test(
      function() {
        assert_true(CSS.supports("font-weight", "bold"));
        assert_true(CSS.supports("font-weight", "700"));
        assert_true(CSS.supports("font-weight", "900"));
        assert_true(CSS.supports("font-weight", "850"));
        assert_true(CSS.supports("font-weight", "850.3"));
        assert_true(CSS.supports("font-stretch", "51%"));
        assert_true(CSS.supports("font-stretch", "199%"));
      },
      'Valid numeric values for font properties used for styling.')
      test(
          function() {
            assert_false(CSS.supports("font-weight", "100 400"));
            assert_false(CSS.supports("font-stretch", "100% 110%"));
            assert_false(CSS.supports("font-stretch", "0%"));
            assert_false(CSS.supports("font-stretch", "100% 150%"));
          },
          'Invalid values for font properties used for styling (especially no ranges).')
}

var faceTests = {
  'weight' : [
    [ "100", "100" ], [ "700", "700" ], [ "900", "900" ], [ "bold", "bold" ],
    [ "normal", "normal" ], [ "100 400", "100 400" ],
    [ "100 101.5", "100 101.5" ]
  ],
  'stretch' : [
    [ "100%", "100%" ],
    [ "110%", "110%" ],
    [ "50% 200%", "50% 200%" ],
    [ "ultra-condensed", "ultra-condensed" ],
    [ "ultra-expanded", "ultra-expanded" ],
  ]
};

var faceInvalidTests = {
  'weight' : [
    '-100 200',
    '100 -200',
    '500 400',
    '100 1001',
    '1001',
    '1000.5',
    '100 200 300',
    'a',
    'a b c',
  ],
  'stretch' : [
    '0%', '60% 70% 80%', 'a%', 'a b c', '0.1', '-60% 80%', 'ultra-expannnned', '50% 0'
  ]
};

function testParseFace() {
  for (var theProperty of Object.keys(faceTests)) {
    for (var faceTest of faceTests[theProperty]) {
      test(() => {
                  var fontFace = new FontFace("testfont", "url()");
                  assert_equals(fontFace[theProperty], "normal");
                  fontFace[theProperty] = faceTest[0];
                  assert_equals(fontFace[theProperty], faceTest[1]);
                },
           'Valid value ' + faceTest[0] + ' matches ' + faceTest[1] + ' for ' +
               theProperty + ' in @font-face.');
    }
  }

  for (var theProperty of Object.keys(faceInvalidTests)) {
    for (var faceTest of faceInvalidTests[theProperty]) {
      test(() => {
                  var fontFace = new FontFace("testfont", "url()");
                  assert_throws("SyntaxError",
                                () => { fontFace[theProperty] = faceTest; },
                                'Value must not be accepted as weight value.');
                },
           'Value ' + faceTest + ' must not be accepted as ' + theProperty +
               ' in @font-face.')
    }
  }
}

window.addEventListener('load', function() {
  testParseStyle();
  testParseFace();
  done();
});
