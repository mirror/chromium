// [Name] SVGRectElement-dom-width-attr.js
// [Expected rendering result] green rect size 200x200 at 0x0 - and a series of PASS messages

description("Tests dynamic updates of the 'width' attribute of the SVGRectElement object")
createSVGTestCase();

var rectElement = createSVGElement("rect");
rectElement.setAttribute("x", "0");
rectElement.setAttribute("y", "0");
rectElement.setAttribute("width", "10");
rectElement.setAttribute("height", "10");
rectElement.setAttribute("fill", "green");

rootSVGElement.appendChild(rectElement);
shouldBeEqualToString("rectElement.getAttribute('width')", "10");

function repaintTest() {
    rectElement.setAttribute("width", "20");
    shouldBeEqualToString("rectElement.getAttribute('width')", "20");
}

var successfullyParsed = true;
