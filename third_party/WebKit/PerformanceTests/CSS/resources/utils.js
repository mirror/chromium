function createDOMTree(node, siblings, depth) {
    for (var i=0; i<siblings; i++) {
        if (depth) {
            var div = document.createElement("div");
            node.appendChild(div);
            createDOMTree(div, siblings, depth-1);
        }
    }
}

function forceStyleRecalc(node) {
    node.offsetTop; //forces style recalc
}
