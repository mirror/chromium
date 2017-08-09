TestRunner.addResult("This tests a utility's ability to parse filter queries.");

var keys = ["key1", "key2"];
var queries = [
    "text",
    "text with spaces",
    "/regex/",
    "/regex/ /another/",
    "/complex\/regex/",
    "/regex/ text",
    "key1:value",
    "-key1:value",
    "key1:/regex/ outerText",
    "key1:\"\" outerText",
    "key1:\"value with spaces\"",
    "key1:\"valueX:valueY\"",
    "key1:\"value outerText",
    "key1:abc\" \" outerText",
    "-key1:value key2:value2",
    "-key1:value -key2:value2",
    "key1:value innerText key2:value2",
    "outerText1 key1:value outerText2",
];

for (var query of queries) {
    var result = TextUtils.TextUtils.parseFilterQuery(query, keys);
    TestRunner.addResult("\nQuery: " + query);
    TestRunner.addResult("Text: " + JSON.stringify(result.text));
    TestRunner.addResult("Regex: " + JSON.stringify(result.regex.map(regex => regex.source)));
    TestRunner.addResult("Filters: " + JSON.stringify(result.filters));
}
TestRunner.completeTest();
