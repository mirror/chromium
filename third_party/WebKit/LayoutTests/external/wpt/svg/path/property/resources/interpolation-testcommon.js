'use strict';
function test_interpolation(settings, expectations) {
  // Returns a timing function that at 0.5 evaluates to progress.
  function timingFunction(progress) {
    if (progress === 0)
      return 'steps(1, end)';
    if (progress === 1)
      return 'steps(1, start)';
    const y = (8 * progress - 1) / 6;
    return 'cubic-bezier(0, ' + y + ', 1, ' + y + ')';
  }

  test(function(){
    assert_true(CSS.supports(settings.property, settings.from), 'Value "' + settings.from + '" is supported by ' + settings.property);
    assert_true(CSS.supports(settings.property, settings.to), 'Value "' + settings.to + '" is supported by ' + settings.property);
  }, '"' + settings.from + '" and "' + settings.to + '" are valid ' + settings.property + ' values');

  for (let i = 0; i < expectations.length; ++i) {
    const progress = expectations[i].at;
    const expectation = expectations[i].expect;
    const animationId = 'anim' + i;

    const stylesheet = document.getElementById('stylesheet');
    const target = document.getElementById('target');
    const reference = document.getElementById('reference');

    test(function(){
      assert_true(CSS.supports(settings.property, expectation), 'Value "' + expectation + '" is supported by ' + settings.property);

      stylesheet.textContent =
        '#target {\n' +
        '  animation: 2s ' + timingFunction(progress) + ' -1s paused ' + animationId + ';\n' +
        '}\n' +
        '@keyframes ' + animationId + ' {\n' +
        '  0% { ' + settings.property + ': ' + settings.from + '; }\n' +
        '  100% { ' + settings.property + ': ' + settings.to + '; }\n' +
        '}\n' +
        '#reference {\n' +
        '  ' + settings.property + ': ' + expectation + ';\n' +
        '}\n';

      assert_equals(getComputedStyle(target)[settings.property], getComputedStyle(reference)[settings.property]);
      assert_equals(getComputedStyle(target)[settings.property], expectation);
    }, 'Animation between "' + settings.from + '" and "' + settings.to + '" at progress ' + progress);
  }
}

function test_no_interpolation(settings) {
  const expectatFrom = [-1, 0, 0.125].map(function (progress) {
    return {at: progress, expect: settings.from};
  });
  const expectatTo = [0.875, 1, 2].map(function (progress) {
    return {at: progress, expect: settings.to};
  });

  test_interpolation(settings, expectatFrom.concat(expectatTo));
}
