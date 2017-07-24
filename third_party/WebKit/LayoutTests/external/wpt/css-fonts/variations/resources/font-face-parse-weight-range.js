setup({ explicit_done: true });

function testParseWeight() {
    test(function() {
        assert_true(CSS.supports("font-weight", "bold"));
        assert_true(CSS.supports("font-weight", "700"));
        assert_true(CSS.supports("font-weight", "900"));
        assert_true(CSS.supports("font-weight", "850"));
        assert_true(CSS.supports("font-weight", "850.3"));
    }, 'Valid values for font-weight used for styling.')
    test(function() {
        assert_false(CSS.supports("font-weight", "100 400"));
    }, 'Invalid values for font-weight used for styling.')
}

var faceWeightTests = [
    ["100", "100"],
    ["700", "700"],
    ["900", "900"],
    ["bold", "bold"],
    ["normal", "normal"],
    ["100 400", "100 400"],
    ["100 101.5", "100 101.5"]
];

var faceWeightInvalidTests = [
    "-100 200",
    "100 -200",
    "500 400",
    "100 1001",
    "1001",
    "1000.5",
    "100 200 300",
    "a",
    "a b c",
];

function testParseWeightFace() {
    for (var weightFaceTest of faceWeightTests) {
        test(() => {
            var fontFace = new FontFace("testfont", "url()");
            assert_equals(fontFace.weight, "normal");
            fontFace.weight = weightFaceTest[0];
            assert_equals(fontFace.weight, weightFaceTest[1]);
        }, 'Valid value ' +  weightFaceTest[0] + ' matches ' + weightFaceTest[1] + ' for font-weight in @font-face.');
    }

    for (var weightFaceTest of faceWeightInvalidTests) {
        test(() => {
            var fontFace = new FontFace("testfont", "url()");
            assert_throws("SyntaxError", () => { fontFace.weight = weightFaceTest; }, 'Value must not be accepted as weight value.');
        }, 'Value ' +weightFaceTest + ' must not be accepted as weight in @font-face.')
    }
}

window.addEventListener('load', function() {
    testParseWeight();
    testParseWeightFace();
    done();
});
