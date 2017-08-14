function collapseWhitespace(str) {
    return str.replace(/\s+/g, ' ').replace(/^\s+|\s+$/g, '');
}

function focusedElementDescription()
{
    var element = accessibilityController.focusedElement;
    var name = collapseWhitespace(element.name);
    var value = element.stringValue.substr(9);
    var description = collapseWhitespace(element.description);
    return name + ', ' +  value + ', ' + description + ', intValue:' + element.intValue + ', range:'+ element.minValue + '-' + element.maxValue;
}

function checkFocusedElementAXAttributes(expected) {
    shouldBeEqualToString('focusedElementDescription()', expected);
}
