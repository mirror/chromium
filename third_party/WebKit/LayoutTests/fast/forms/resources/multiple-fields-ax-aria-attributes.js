function focusedElementDescription()
{
    var element = accessibilityController.focusedElement;
    var name = element.name.trim();
    var value = element.stringValue.substr(9);
    var description = element.description.trim();
    return name + ', ' +  value + ', ' + description + ', intValue:' + element.intValue + ', range:'+ element.minValue + '-' + element.maxValue;
}

function checkFocusedElementAXAttributes(expected) {
    shouldBeEqualToString('focusedElementDescription()', expected);
}
