function focusedElementDescription()
{
    var element = accessibilityController.focusedElement;
    var value = element.stringValue.substr(9);
    return element.name + ', ' +  value + ', intValue:' + element.intValue + ', range:'+ element.minValue + '-' + element.maxValue;
}

function checkFocusedElementAXAttributes(expected) {
    shouldBeEqualToString('focusedElementDescription()', expected);
}
