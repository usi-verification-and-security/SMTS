// Check if two array have same content
function arrayEqual(a1, a2) {
    if (a1.length !== a2.length) {
        return false;
    }
    for (let i = 0; i < a1.length; ++i) {
        if (a1[i] !== a2[i]) {
            return false;
        }
    }
    return true;
}


// Check if targetNode name is equal to at least one of the nodes node name
function isNodeInNodes(targetNode, nodes) {
    for (let node of nodes) {
        if (arrayEqual(targetNode.name, node.name)) {
            return true;
        }
    }
    return false;
}

function isNotNodeInNodes(targetNode, nodes) {
    return !isNodeInNodes(targetNode, nodes);
}