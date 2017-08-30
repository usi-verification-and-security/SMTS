// Set of functions that manipulate the DOM object 'smts-data-container' and
// manage its content.
smts.data = {

    // Make a button for requesting CNF of currently selected node
    // @return {HtmlNode}: The HTML button.
    makeGetClausesBtn: function() {
        let getClausesBtn = document.createElement('div');
        getClausesBtn.id = 'smts-data-get-clauses';
        getClausesBtn.className = 'btn btn-default btn-xs smts-hide-on-mode-database';
        getClausesBtn.innerHTML = 'Get Clauses';
        let instanceName =  smts.instances.getSelected();
        let nodePath = JSON.stringify(smts.tree.getSelectedNodes()[0].name);
        getClausesBtn.addEventListener('click', () => smts.cnf.load(instanceName, nodePath, null));
        return getClausesBtn;
    },

    // Make an event object with the wanted attributes for the data table
    // @param {TreeManager.Node} node: The mold event.
    // @return {Object}: The object representing the event, to be put in
    // the data table.
    makeItemEvent: function(event) {
        let itemEvent = {};
        if (event && event.data) {
            for (let key in event.data) {
                if (key !== 'name' && key !== 'node') {
                    itemEvent[key] = event.data[key];
                }
            }
        }
        return itemEvent;
    },

    // Make a node object with the wanted attributes for the data table
    // @param {TreeManager.Node} node: The mold node.
    // @return {Object}: The object representing the node, to be put in the
    // data table.
    makeItemNode: function(node) {
        let itemNode = {};
        if (node) {
            itemNode.status = node.status;
            itemNode.solvers = node.solvers.length;
            itemNode.balanceness = node.getBalanceness();
            if (node.info) {
                for (let key in node.info) {
                    itemNode[key] = node.info[key];
                }
            }
        }
        return itemNode;
    },

    // Make a solver object with the wanted attributes for the data table
    // @param {TreeManager.Solver} solver: The mold solver.
    // @return {Object}: The object representing the solver, to be put in
    // the data table.
    makeItemSolver: function(solver) {
        let itemSolver = {};
        if (solver) {
            // Do nothing
        }
        return itemSolver;
    },

    // Make table containing an object
    // It creates a DOM table element, with first colon being the key/index
    // of the given object/array, and the second the value. The function is
    // recursive, meaning if a value is an object/array, another table will
    // be created inside the cell.
    // @param {Object} item: The object to b converted into a HTML table.
    // @return {HtmlElement}: A DOM table element representing the object.
    makeTable: function(item) {
        // If item is an object or an array, create a table element
        if (item && typeof item === 'object') {
            let table = document.createElement('table');
            table.id = 'smts-data-table';
            table.classList.add('smts-table');

            // Make header, only if object/array has keys/elements
            if (Object.keys(item).length > 0) {
                let header = table.createTHead().insertRow(0);
                header.appendChild(document.createElement('th')).innerText = 'Key';
                header.appendChild(document.createElement('th')).innerText = 'Value';
            }

            // Populate table
            let body = table.appendChild(document.createElement('tbody'));
            for (let key in item) {
                let row = body.appendChild(document.createElement('tr'));
                row.insertCell(0).innerText = key;
                row.insertCell(1).appendChild(this.makeTable(item[key]));
            }

            return table;
        }

        // If item is a number, boolean or string, create a simple element
        let element = document.createElement('span');
        element.innerText = item;
        return element;
    },

    // Replace data table and change title of container
    // @param {TreeManager.*} item: A solver, event or node. The item will
    // be represented in the table.
    // @param {String} itemType: The type of the item. It can be one of:
    // 'solver', 'node', 'event +', 'event -', 'event status', 'event and',
    // 'event or' or 'event solved'.
    update: function(item, itemType) {
        document.getElementById('smts-data-title').innerHTML = itemType;
        let dataTableContainer = document.getElementById('smts-data-table-container');

        // Reset container
        dataTableContainer.innerHTML = '';

        // Create object to put in data table
        let dataTableItem = {};

        switch (itemType) {
            case 'event':
                dataTableItem = this.makeItemEvent(item);
                break;

            case 'node':
                dataTableItem = this.makeItemNode(item);
                dataTableContainer.appendChild(this.makeGetClausesBtn());
                break;

            case 'solver':
                dataTableItem = this.makeItemSolver(item);
                break;
        }

        // Create and insert new table
        let dataTable = this.makeTable(dataTableItem);
        dataTableContainer.appendChild(dataTable);
    }
};