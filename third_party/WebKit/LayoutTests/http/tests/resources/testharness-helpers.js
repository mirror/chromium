/*
 * testharness-helpers contains various useful extensions to testharness.js to
 * allow them to be used across multiple tests before they have been
 * upstreamed. This file is intended to be usable from both document and worker
 * environments, so code should for example not rely on the DOM.
 */

// Returns a promise that fulfills after the provided |promise| is fulfilled.
// The |test| succeeds only if |promise| rejects with an exception matching
// |code|. Accepted values for |code| follow those accepted for assert_throws().
// The optional |description| describes the test being performed.
//
// E.g.:
//   assert_promise_rejects(
//       new Promise(...), // something that should throw an exception.
//       'NotFoundError',
//       'Should throw NotFoundError.');
//
//   assert_promise_rejects(
//       new Promise(...),
//       new TypeError(),
//       'Should throw TypeError');
function assert_promise_rejects(promise, code, description) {
  return promise.then(
    function() {
      throw 'assert_promise_rejects: ' + description + ' Promise did not reject.';
    },
    function(e) {
      if (code !== undefined) {
        assert_throws(code, function() { throw e; }, description);
      }
    });
}

// Equivalent to testharness.js's assert_object_equals(), but correctly
// tests that property ownership is the same.
// TODO(jsbell): Upstream this to assert_object_equals and remove.
function assert_object_equals_fixed(actual, expected, description)
{
    function check_equal(actual, expected, stack)
    {
        stack.push(actual);
        var p;
        for (p in actual) {
            assert_equals(expected.hasOwnProperty(p), actual.hasOwnProperty(p),
                          "different property ownership: " + p + " - " + description);

            if (typeof actual[p] === "object" && actual[p] !== null) {
                if (stack.indexOf(actual[p]) === -1) {
                    check_equal(actual[p], expected[p], stack);
                }
            } else {
                assert_equals(actual[p], expected[p], description);
            }
        }
        for (p in expected) {
            assert_equals(expected.hasOwnProperty(p), actual.hasOwnProperty(p),
                          "different property ownership: " + p + " - " + description);
        }
        stack.pop();
    }
    check_equal(actual, expected, []);
}

// Equivalent to assert_in_array, but uses the object-aware equivalence relation
// provided by assert_object_equals_fixed().
function assert_object_in_array(actual, expected_array, description) {
  assert_true(expected_array.some(function(element) {
      try {
        assert_object_equals_fixed(actual, element);
        return true;
      } catch (e) {
        return false;
      }
    }), description);
}

// Assert that the two arrays |actual| and |expected| contain the same set of
// elements as determined by assert_object_equals_fixed. The order is not significant.
//
// |expected| is assumed to not contain any duplicates as determined by
// assert_object_equals_fixed().
function assert_array_equivalent(actual, expected, description) {
  assert_true(Array.isArray(actual), description);
  assert_equals(actual.length, expected.length, description);
  expected.forEach(function(expected_element) {
      // assert_in_array treats the first argument as being 'actual', and the
      // second as being 'expected array'. We are switching them around because
      // we want to be resilient against the |actual| array containing
      // duplicates.
      assert_object_in_array(expected_element, actual, description);
    });
}

// Asserts that two arrays |actual| and |expected| contain the same set of
// elements as determined by assert_object_equals(). The corresponding elements
// must occupy corresponding indices in their respective arrays.
function assert_array_objects_equals(actual, expected, description) {
  assert_true(Array.isArray(actual), description);
  assert_equals(actual.length, expected.length, description);
  actual.forEach(function(value, index) {
      assert_object_equals(value, expected[index],
                           description + ' : object[' + index + ']');
    });
}

// Asserts that |object| that is an instance of some interface has the attribute
// |attribute_name| following the conditions specified by WebIDL, but it's
// acceptable that the attribute |attribute_name| is an own property of the
// object because we're in the middle of moving the attribute to a prototype
// chain.  Once we complete the transition to prototype chains,
// assert_will_be_idl_attribute must be replaced with assert_idl_attribute
// defined in testharness.js.
//
// FIXME: Remove assert_will_be_idl_attribute once we complete the transition
// of moving the DOM attributes to prototype chains.  (http://crbug.com/43394)
function assert_will_be_idl_attribute(object, attribute_name, description) {
  assert_true(typeof object === "object", description);

  assert_true("hasOwnProperty" in object, description);

  // Do not test if |attribute_name| is not an own property because
  // |attribute_name| is in the middle of the transition to a prototype
  // chain.  (http://crbug.com/43394)

  assert_true(attribute_name in object, description);
}

// Stringifies a DOM object.  This function stringifies not only own properties
// but also DOM attributes which are on a prototype chain.  Note that
// JSON.stringify only stringifies own properties.
function stringifyDOMObject(object)
{
    function deepCopy(src) {
        if (typeof src != "object")
            return src;
        var dst = Array.isArray(src) ? [] : {};
        for (var property in src) {
            dst[property] = deepCopy(src[property]);
        }
        return dst;
    }
    return JSON.stringify(deepCopy(object));
}

(function() {
  var promise_tests = Promise.resolve();
  // Helper function to run promise tests one after the other.
  // TODO(ortuno): Remove once https://github.com/w3c/testharness.js/pull/115/files
  // gets through.
  function sequential_promise_test(func, name) {
    var test = async_test(name);
    promise_tests = promise_tests.then(function() {
      return test.step(func, test, test);
    }).then(function() {
      test.done();
    }).catch(test.step_func(function(value) {
      // step_func catches the error again so the error doesn't propagate.
      throw value;
    }));
  }

  self.sequential_promise_test = sequential_promise_test;
})();
