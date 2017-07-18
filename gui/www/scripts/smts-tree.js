// Set of functions that manipulate the DOM object 'smts-tree'
smts.tree = {

    // Get the position of the tree
    // @return {String}: The position in the form 'translate(174,150) scale(1)'
    // or `null` if the tree is not present.
    getPosition: function() {
        let tree = document.getElementById('smts-tree');
        return tree ? tree.getAttribute('transform') : null;
    }
};