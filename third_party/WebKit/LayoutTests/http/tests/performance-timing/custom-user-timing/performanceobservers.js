// Compares a performance entry to a predefined one.
// perfEntriesToCheck is an array of performance entries from the user agent
// expectedEntries is an array of performance entries minted by the test.
function checkEntries(perfEntriesToCheck, expectedEntries) {
  function findMatch(pe) {
    // We match based on entryType, name, detail(if listed)
    // startTime(if listed).
    for (let i = expectedEntries.length - 1; i >= 0; i--) {
      const ex = expectedEntries[i];
      if (ex.entryType === pe.entryType && ex.name === pe.name &&
          (ex.detail === undefined ||
              JSON.stringify(ex.detail) === JSON.stringify(pe.detail)) &&
          (ex.startTime === undefined || ex.startTime === pe.startTime)) {
        return ex;
      }
    }
    return null;
  }

  assert_equals(perfEntriesToCheck.length, expectedEntries.length,
      "performance entries must match");

  perfEntriesToCheck.forEach((pe1) => {
    assert_not_equals(findMatch(pe1), null, "Entry matches");
  });
}