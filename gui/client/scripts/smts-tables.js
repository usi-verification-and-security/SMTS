// Set of functions that manipulate the tables DOM objects
smts.tables = {

    // Set of functions that manipulate the DOM object 'smts-data-container'
    data: {

        // Make an event object with the wanted attributes for the data table
        // @param {TreeManager.Event} event: The mold event.
        // @return {Object}: The object representing the event, to be put in
        // the data table.
        makeItemEvent: function(event) {
            return event.data;
        },

        // Make a node object with the wanted attributes for the data table
        // @param {TreeManager.Node} node: The mold node.
        // @return {Object}: The object representing the node, to be put in the
        // data table.
        makeItemNode: function(node) {
            let itemNode = {};
            // Transform to string or it will show and array
            itemNode.name = JSON.stringify(node.name);
            itemNode.type = node.type;
            itemNode.solvers = node.solvers;
            itemNode.status = node.status;
            itemNode.balanceness = node.getBalanceness();
            return itemNode;
        },

        // Make a solver object with the wanted attributes for the data table
        // @param {TreeManager.Solver} solver: The mold solver.
        // @return {Object}: The object representing the solver, to be put in
        // the data table.
        makeItemSolver: function(solver) {
            let itemSolver = {};
            itemSolver.name = solver.name;
            // Transform to string or it will show an array
            itemSolver.node = JSON.stringify(solver.node);
            itemSolver.data = solver.data;
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
                case 'solver':
                    dataTableItem = this.makeItemSolver(item);
                    break;

                case 'node':
                    dataTableItem = this.makeItemNode(item);
                    break;

                case 'event STATUS':
                case 'event SOLVED':
                case 'event AND':
                case 'event OR':
                case 'event +':
                case 'event -':
                    dataTableItem = this.makeItemEvent(item);
                    break;
            }

            // Create and insert new table
            let dataTable = this.makeTable(dataTableItem);
            dataTableContainer.appendChild(dataTable);
        }

    },


    // Set of functions that manipulate the DOM object 'smts-events-container'
    events: {

        // {String} queryRows: String to shorten queries.
        queryRows: '#smts-events-table > tbody > tr',

        // Make cells of events table with selected nodes bold
        // @param {Node[]} selectedNodes: List of nodes for which corresponding
        // events' cells have to be selected.
        boldSelected: function(selectedNodes) {
            // Remove bold to all event cells
            let rows = document.querySelectorAll(`${this.queryRows} > td.smts-bold`);
            rows.forEach(row => row.classList.remove('smts-bold'));
            // Apply bold to event cells with selected nodes
            for (let selectedNode of selectedNodes) {
                let selectedNodeNameStr = JSON.stringify(selectedNode.name);
                // node
                rows = document.querySelectorAll(`${this.queryRows}[data-node="${selectedNodeNameStr}"]`);
                if (rows) rows.forEach(row => row.children[2].classList.add('smts-bold'));
                // data-node
                rows = document.querySelectorAll(`${this.queryRows}[data-data-node="${selectedNodeNameStr}"]`);
                if (rows) rows.forEach(row => row.children[4].classList.add('smts-bold'));
            }
        },

        // Hide all rows in events table
        hideAll: function() {
            let rows = document.querySelectorAll(this.queryRows);
            rows.forEach(row => row.classList.add('smts-hidden'));
        },

        // Highlight all rows with event matching one of events
        // @param {TreeManager.Events[]} events: List of events to be
        // highlighted.
        highlight: function(events) {
            // Remove highlight from all events
            let rows = document.querySelectorAll(this.queryRows);
            rows.forEach(row => row.classList.remove('smts-highlight'));
            // Highlight selected nodes
            for (let event of events) {
                rows = document.querySelectorAll(`${this.queryRows}[data-event="${event.id}"]`);
                if (rows) rows.forEach(row => row.classList.add('smts-highlight'));
            }
        },

        // Check if a tab of the events container is selected
        // @param {String} tabName: the name of the tab. Has to be lowercase
        // for HTML id compatibility.
        // @return {Boolean}: `true` if the tab is active, `false` otherwise.
        isTabActive: function(tabName) {
            let tab = document.getElementById(`smts-events-navbar-${tabName}`);
            return tab ? tab.classList.contains('active') : false;
        },

        // Show all rows in events table
        showAll: function() {
            let rows = document.querySelectorAll(this.queryRows);
            if (rows) rows.forEach(row => row.classList.remove('smts-hidden'));
        },

        // Show rows of events table for which the event node matches at least
        // one of the selected nodes
        // @param {Node[]} selectedNodes: List of nodes for which corresponding
        // events have to be selected.
        showSelected: function(selectedNodes) {
            // Hide all nodes
            this.hideAll();
            // Show nodes that are in event.node or event.data.node
            for (let selectedNode of selectedNodes) {
                let selectedNodeNameStr = JSON.stringify(selectedNode.name);
                // node
                let rows = document.querySelectorAll(`${this.queryRows}[data-node="${selectedNodeNameStr}"]`);
                if (rows) rows.forEach(row => row.classList.remove('smts-hidden'));
                // data-node
                rows = document.querySelectorAll(`${this.queryRows}[data-data-node="${selectedNodeNameStr}"]`);
                if (rows) rows.forEach(row => row.classList.remove('smts-hidden'));
            }
        },

        // Show all or selected nodes of events table, depending on selected tab
        // @param {Node[]} selectedNodes: List of nodes to compare to each
        // row's node.
        update: function(selectedNodes) {
            this.boldSelected(selectedNodes);
            this.isTabActive('selected') ? this.showSelected(selectedNodes) : this.showAll();
        }
    },


    // Set of functions that manipulate the DOM object 'smts-instances-container'
    instances: {

        // Highlight all rows with instance matching one of instances
        // @param {Instance} instance: Instance to be highlighted.
        // TODO: make `instance` become `instances`
        highlight: function(instance) {
            let rows = document.querySelectorAll('#smts-instances-table > tbody > tr');
            if (rows) {
                for (let row of rows) {
                    let instanceName = row.children[0].innerHTML;
                    if (instanceName && instanceName === instance.name) {
                        row.classList.add('smts-highlight');
                    }
                    else {
                        row.classList.remove('smts-highlight');
                    }
                }
            }
        }
    },


    // Set of functions that manipulate the DOM object 'smts-solvers-container'
    solvers: {

        // Highlight all rows with solver matching one of solvers
        // @param {TreeManager.Solver[]} solvers: List of solvers to be
        // highlighted.
        highlight: function(solvers) {
            let rows = document.querySelectorAll('#smts-solvers-table > tbody > tr');
            if (rows) {
                for (let row of rows) {
                    let solverName = row.children[0].innerHTML;
                    if (solverName && (new TreeManager.Solver(solverName)).equalAny(solvers)) {
                        row.classList.add('smts-highlight');
                    }
                    else {
                        row.classList.remove('smts-highlight');
                    }
                }
            }
        },

        // Check if a tab of the solvers container is selected
        // @param {String} tabName: The name of the tab. Has to be lowercase
        // for HTML id compatibility.
        // @return {Boolean}: `true` if the tab is active, `false` otherwise.
        isTabActive: function(tabName) {
            let tab = document.getElementById(`smts-solvers-navbar-${tabName}`);
            if (!tab) {
                return false;
            }
            return tab.classList.contains('active');
        },

        // Show all rows in solvers table
        showAll: function() {
            let rows = document.querySelectorAll('#smts-solvers-table > tbody > tr');
            for (let row of rows) {
                row.classList.remove('smts-hidden');
            }
        },

        // Show rows of solvers table for which the solver node matches at
        // least one of the selected nodes
        // @param {Node[]} selectedNodes: List of nodes to compare to each
        // row's node.
        showSelected: function(selectedNodes) {
            let rows = document.querySelectorAll('#smts-solvers-table > tbody > tr');
            for (let row of rows) {
                let nodeName = row.children[1].innerHTML;
                if (nodeName && (new TreeManager.Node(JSON.parse(nodeName), '')).equalAny(selectedNodes)) {
                    row.classList.remove('smts-hidden');
                }
                else {
                    row.classList.add('smts-hidden');
                }
            }
        },

        // Show all or selected nodes of solvers table, depending on selected tab
        // @param {Node[]} selectedNodes: List of nodes to compare to each
        // row's node.
        update: function(selectedNodes) {
            if (this.isTabActive('selected')) {
                this.showSelected(selectedNodes);
            } else {
                this.showAll();
            }
        }
    }
};