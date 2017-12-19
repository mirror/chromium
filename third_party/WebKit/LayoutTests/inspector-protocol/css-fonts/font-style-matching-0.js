(async function(testRunner) {
  var {page, session, dp} = await testRunner.startHTML(`<!DOCTYPE html>
<html>
<meta charset="UTF-8">
<head>
    <!-- available fonts --->
    <style type="text/css">
    </style>
    <link rel="stylesheet" type="text/css" href="${testRunner.url("resources/applied-styles.css")}">
</head>
<body>
<pre>According to the CSS3 Fonts Module, Step 4 or the Font Style Matching Algorithm
( https://drafts.csswg.org/css-fonts-3/#font-style-matching ) must narrow down the
available font faces by finding the nearest match in the following order of
precedence: stretch, style, weight.
</pre>
    <div class="test">
        <div id="condensed_normal_100">abcdefg</div>
        <div id="condensed_normal_900">abcdefg</div>
        <div id="condensed_italic_100">abcdefg</div>
        <div id="condensed_italic_900">abcdefg</div>
        <div id="expanded_normal_100">abcdefg</div>
        <div id="expanded_normal_900">abcdefg</div>
        <div id="expanded_italic_100">abcdefg</div>
        <div id="expanded_italic_900">abcdefg</div>
    </div>
</body>
</html>
`, 'The test verifies functionality of protocol method CSS.addRule.');

  await dp.DOM.enable();
  await dp.CSS.enable();

  var test = await testRunner.loadScript('resources/style-matching-test.js');
  test(testRunner, session, dp);
  
  var CSSHelper = await testRunner.loadScript('../resources/css-helper.js');
  var cssHelper = new CSSHelper(testRunner, dp);
})
