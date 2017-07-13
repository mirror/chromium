function createDOMTree(node, siblings, depth) {
    if (!depth)
        return;
    for (var i=0; i<siblings; i++) {
        var div = document.createElement("div");
        node.appendChild(div);
        createDOMTree(div, siblings, depth-1);
    }
}

function forceStyleRecalc(node) {
    node.offsetTop; // forces style recalc
}

function createDeepDOMTree() {
        createDOMTree(document.body, 2, 15);
}

function createShallowDOMTree() {
        createDOMTree(document.body, 2, 15);
}