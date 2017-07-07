description("This test exercises the 'font-style' property in CSS computed styles.");

var testDiv = document.createElement('div');
document.body.appendChild(testDiv);

function computedFontStyle(fontStyleString) {
    testDiv.style.fontStyle = 'oblique';
    testDiv.style.fontStyle = fontStyleString;
    return window.getComputedStyle(testDiv).getPropertyValue('font-style');
}

shouldBe("computedFontStyle('normal')", "'normal'");
shouldBe("computedFontStyle('oblique')", "'italic'");
shouldBe("computedFontStyle('italic')", "'italic'");

// TODO(drott) crbug.com/739139 Activate those, once the parser understands the
// oblique <angle> syntax.
// shouldBe("computedFontStyle('oblique 20')", "'italic'");
// shouldBe("computedFontStyle('oblique 19')", "'oblique 19'");
// shouldBe("computedFontStyle('oblique 21')", "'oblique 21'");
