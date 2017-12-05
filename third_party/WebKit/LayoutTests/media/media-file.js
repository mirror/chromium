function setSrcByTagName(tagName, src) {
    var elements = document.getElementsByTagName(tagName);
    if (elements) {
        for (var i = 0; i < elements.length; ++i)
            elements[i].src = src;
    }
}

function setSrcById(id, src) {
    var element = document.getElementById(id);
    if (element)
        element.src = src;
}
