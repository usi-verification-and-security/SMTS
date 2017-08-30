// Set of functions that manipulate the DOM object 'smts-solvers-container'
smts.solvers = {

    // Make cells of solvers table with selected nodes bold
    // @param {Node[]} selectedNodes: List of nodes for which corresponding
    // solvers' cells have to be selected.
    boldSelected: function(selectedNodes) {
        // Remove bold from all event cells
        let rows = this.getRows('>td.smts-bold');
        if (rows) rows.forEach(row => row.classList.remove('smts-bold'));

        // Apply bold to solver cells with selected nodes
        for (let selectedNode of selectedNodes) {
            let selectedNodeNameStr = JSON.stringify(selectedNode.name);
            rows = this.getRows(`[data-node="${selectedNodeNameStr}"]`);
            if (rows) rows.forEach(row => row.children[1].classList.add('smts-bold'));
        }
    },

    // Get rows of solvers table
    // @params {String} [optional] option: An extension to the original
    // query. If no option is provided, all rows are selected. If an option
    // is given, only rows matching the option are selected.
    // E.g.: option = '[data-node="[0,0]"]' selects only rows with
    // attribute `data-node` matching "[0.,0]".
    getRows: function(option = '') {
        let queryRows = '#smts-solvers-table > tbody > tr';
        return document.querySelectorAll(`${queryRows}${option}`)
    },

    // Get name of selected solver
    // @return {string}: Name of the selected solver, the empty string if
    // no solver is selected.
    getSelected: function() {
        let rows = this.getRows('.smts-highlight'); // Get selected
        if (rows && rows[0]) {
            console.log(rows[0].getAttribute('data-solver'));
            return rows[0].getAttribute('data-solver');
        }
        console.log('NOTHING');
        return '';
    },

    // Highlight all rows with solver matching one of solvers
    // @param {TreeManager.Solver[]} solvers: List of solvers to be
    // highlighted.
    highlight: function(solvers) {
        // Remove highlight from all solvers
        let rows = this.getRows();
        if (rows) rows.forEach(row => row.classList.remove('smts-highlight'));

        // Highlight selected nodes
        for (let solver of solvers) {
            rows = this.getRows(`[data-solver='${solver.name}']`);
            if (rows) rows.forEach(row => row.classList.add('smts-highlight'));
        }
    },

    // Check if a tab of the solvers container is selected
    // @param {String} tabName: The name of the tab. Has to be lowercase
    // for HTML id compatibility.
    // @return {Boolean}: `true` if the tab is active, `false` otherwise.
    isTabActive: function(tabName) {
        let tab = document.getElementById(`smts-solvers-navbar-${tabName}`);
        return tab ? tab.classList.contains('active') : false;
    },

    // Show all rows in solvers table
    showAll: function() {
        let rows = this.getRows();
        if (rows) rows.forEach(row => row.classList.remove('smts-hidden'));
    },

    // Show rows of solvers table for which the solver node matches at
    // least one of the selected nodes
    // @param {Node[]} selectedNodes: List of nodes to compare to each
    // row's node.
    showSelected: function(selectedNodes) {
        // Hide all nodes
        let rows = this.getRows();
        if (rows) rows.forEach(row => row.classList.add('smts-hidden'));

        // Show nodes that are in solver.node
        for (let selectedNode of selectedNodes) {
            let selectedNodeNameStr = JSON.stringify(selectedNode.name);
            rows = this.getRows(`[data-node="${selectedNodeNameStr}"]`);
            if (rows) rows.forEach(row => row.classList.remove('smts-hidden'));
        }
    },

    // Show all or selected nodes of solvers table, depending on selected tab
    // @param {Node[]} selectedNodes: List of nodes to compare to each
    // row's node.
    update: function(selectedNodes) {
        this.boldSelected(selectedNodes);
        this.isTabActive('selected') ? this.showSelected(selectedNodes) : this.showAll();
    }
};