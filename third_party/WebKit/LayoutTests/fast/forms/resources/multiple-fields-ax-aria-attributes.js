function focusedElementDescription()
{
    var element = accessibilityController.focusedElement;
    var name = element.name;
    var value = element.stringValue.substr(9);
    var description = element.description;
    return name + ', ' +  value + ', ' + description + ', intValue:' + element.intValue + ', range:'+ element.minValue + '-' + element.maxValue;
}

function checkFocusedElementAXAttributes(expected) {
    shouldBeEqualToString('focusedElementDescription()', expected);
}
