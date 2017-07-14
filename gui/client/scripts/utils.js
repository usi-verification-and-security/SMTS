const tables = {

    events: {

        // Highlight all rows with event matching one of events
        // @param {TreeManager.Solver[]} events: List of solvers to be
        // compared to.
        highlight: function(events) {
            let rows = document.querySelectorAll('#smts-events-table > tbody > tr');
            if (rows) {
                for (let row of rows) {
                    let eventName = row.children[0].innerHTML;
                    if (eventName && (new TreeManager.Event({name: eventName})).equalAny(events)) {
                        row.classList.add('smts-highlight');
                    }
                    else {
                        row.classList.remove('smts-highlight');
                    }
                }
            }
        },

        // Check if a tab of the events container is selected
        // @param {String} tabName: the name of the tab. Has to be lowercase
        // for HTML id compatibility.
        // @return {Boolean}: true if the tab is active, false otherwise.
        isTabActive: function(tabName) {
            let tab = document.getElementById(`smts-events-navbar-${tabName}`);
            if (!tab) {
                return false;
            }
            return tab.classList.contains('active');
        },

        // Show all rows in events table
        showAll: function() {
            let rows = document.querySelectorAll('#smts-events-table > tbody > tr');
            for (let row of rows) {
                row.classList.remove('smts-hidden');
            }
        },

        // Show rows of events table for which the event node matches at least
        // one of the selected nodes
        // @param {Node[]} selectedNodes: list of nodes to compare to each
        // row's node.
        showSelected: function(selectedNodes) {
            let rows = document.querySelectorAll('#smts-events-table > tbody > tr');
            for (let row of rows) {
                let nodeName = row.children[2].innerHTML;
                if (nodeName && (new TreeManager.Node(JSON.parse(nodeName), '')).equalAny(selectedNodes)) {
                    row.classList.remove('smts-hidden');
                }
                else {
                    row.classList.add('smts-hidden');
                }
            }
        },

        // Show all or selected nodes of events table, depending on selected tab
        // @param {Node[]} selectedNodes: list of nodes to compare to each
        // row's node.
        update: function(selectedNodes) {
            if (this.isTabActive('selected')) {
                this.showSelected(selectedNodes);
            } else {
                this.showAll();
            }
        }
    },

    solvers: {

        // Highlight all rows with solver matching one of solvers
        // @param {TreeManager.Solver[]} solvers: List of solvers to be
        // compared to.
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
        // @param {String} tabName: the name of the tab. Has to be lowercase
        // for HTML id compatibility.
        // @return {Boolean}: true if the tab is active, false otherwise.
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
        // @param {Node[]} selectedNodes: list of nodes to compare to each
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
        // @param {Node[]} selectedNodes: list of nodes to compare to each
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