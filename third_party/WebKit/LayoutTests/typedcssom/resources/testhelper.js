const gStyleValueAttributes = {
  CSSKeywordValue: ['value']
};

let Matchers = {};
for (const [name, attributes] of Object.entries(gStyleValueAttributes)) {
  const matcher_name = name.startsWith('CSS') ? name.slice(3) : name;
  Matchers[matcher_name] = (...args) => {
    const attributes = gStyleValueAttributes[name];
    let matcher = { name: name };
    for (let i = 0; i < attributes.length; i++) {
      matcher[attributes[i]] = args[i];
    }
    return matcher;
  };
}

function value_repr(value) {
  if (value == null)
    return 'null';

  if (typeof(value) == 'string')
    return '"' + value + '"';

  if (value instanceof Array)
    return '[' + value.map(value_repr).join(', ') + ']';

  if (typeof(value) == 'object') {
    const class_name = value.constructor == Object ? value.name : value.constructor.name;
    const attributes = gStyleValueAttributes[class_name];

    if (!attributes)
      return class_name + '()';

    return class_name + '(' + attributes.map(attr => attr + '=' + value_repr(value[attr])).join(', ') + ')';
  }
}

function assert_matches(value, matcher) {
  assert_true(value_matches(value, matcher),
    'Expected ' + value_repr(value) + ', but got ' + value_repr(matcher));
}

function value_matches(value, matcher) {
  if (value == null && matcher == null)
    return true;
  if (value == null || matcher == null)
    return false;

  if (typeof(value) == 'string' && typeof(matcher) == 'string')
    return value === matcher;

  if (value instanceof Array && matcher instanceof Array) {
    if (value.length != matcher.length)
      return false;

    for (let i = 0; i < value.length; i++) {
      if (!value_matches(value[i], matcher[i]))
        return false;
    }

    return true;
  }

  if (typeof(value) == 'object' && typeof(matcher) == 'object') {
    const class_name = value.constructor.name;
    if (class_name != matcher.name)
      return false;

    const attributes = gStyleValueAttributes[class_name];
    if (!attributes)
      return false;

    for (const attribute of attributes) {
      if (!value_matches(value[attribute], matcher[attribute]))
        return false;
    }

    return true;
  }

  return false;
}

/*

// Compares two arrays of CSSStyleValues to check if every element is equal
function assert_style_value_array_equals(a, b) {
  assert_equals(a.length, b.length);
  for (let i = 0; i < a.length; i++) {
    assert_style_value_equals(a[i], b[i]);
  }
}
*/
const gValidUnits = [
  'number', 'percent', 'em', 'ex', 'ch',
  'ic', 'rem', 'lh', 'rlh', 'vw',
  'vh', 'vi', 'vb', 'vmin', 'vmax',
  'cm', 'mm', 'Q', 'in', 'pt',
  'pc', 'px', 'deg', 'grad', 'rad',
  'turn', 's', 'ms', 'Hz', 'kHz',
  'dpi', 'dpcm', 'dppx', 'fr',
];

// Creates a new div element with specified inline style |cssText|.
// The created element is deleted during test cleanup.
function createDivWithStyle(test, cssText) {
  let element = document.createElement('div');
  element.style = cssText || '';
  document.body.appendChild(element);
  test.add_cleanup(() => {
    element.remove();
  });
  return element;
}

// Creates a new div element with inline style |cssText| and returns
// its inline style property map.
function createInlineStyleMap(test, cssText) {
  return createDivWithStyle(test, cssText).attributeStyleMap;
}

// Creates a new div element with inline style |cssText| and returns
// its computed style property map.
function createComputedStyleMap(test, cssText) {
  return createDivWithStyle(test, cssText).computedStyleMap();
}
