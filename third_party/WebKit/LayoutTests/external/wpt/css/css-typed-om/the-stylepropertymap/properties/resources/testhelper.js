function createDiv(test) {
  let element = document.createElement('div');
  document.body.appendChild(element);
  test.add_cleanup(() => {
    element.remove();
  });
  return element;
}

function testSet(propertyName, values, description) {
  test(t => {
    let element = createDiv(t);
    let styleMap = element.attributeStyleMap;

    for (const value of values) {
      styleMap.set(propertyName, value);
      getComputedStyle(element); // Force a style recalc.
      assert_equals(element.style[propertyName], value.toString());
    }
  }, `Can set '${propertyName}' in a style map to ${description}`);
}

function runPropertyTests(propertyName, validValues) {
  for (const validValue of validValues) {
    if (validValue == '0') {
      // unitless zero
    } else if (validValue === '<length>') {
      testSet(propertyName, [
        new CSSUnitValue(0, 'px'),
        new CSSUnitValue(-3.14, 'em'),
        new CSSUnitValue(3.14, 'cm'),
      ], 'a length CSSUnitValue');
    } else if (validValue == '<percentage>') {
      testSet(propertyName, [
        new CSSUnitValue(0, 'percent'),
        new CSSUnitValue(-3.14, 'percent'),
        new CSSUnitValue(3.14, 'percent'),
      ], 'a percent CSSUnitValue');
    } else {
      // We assume that it's a keyword value.
      testSet(propertyName, [
        new CSSKeywordValue(validValue),
      ], `a CSSKeywordValue with value ${validValue}`);
    }
  }
}
