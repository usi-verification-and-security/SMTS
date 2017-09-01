// Set of functions that manipulate the DOM object 'smts-events-container'
smts.events = {

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // VARIABLES
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // {number}: Index of current event selected in events table
    index: -1,

    // {TreeManager.Event[]}: List of events to be shown in the events table
    events: [],


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // EVENTS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Append events to existing ones
    // @param {TreeManager.Event[]}: The events to append.
    append: function(events) {
        // Transform objects into TreeManager.Event
        if (events) {
            let startTime = this.events.length > 0
                ? this.events[0].ts
                : events[0].ts;
            for (let event of events) {
                this.events.push(new TreeManager.Event(event, startTime));
            }
        }
    },

    // Get all events
    // @return {TreeManager.Event[]}: The events.
    get: function() {
        return this.events;
    },

    // Tell if last event is the selected one.
    // @return {boolean}: `true` if last event is the selected one, `false`
    // otherwise.
    isLastSelected: function() {
        return this.index === this.events.length - 1;
    },

    // Reset events
    reset: function() {
        this.events.length = 0;
        this.index = -1;
    },

    // Set events
    // @param {object[]}: The events.
    set: function(events) {
        this.events.length = 0;
        this.append(events);
    },

    // Get last event
    // @return {TreeManager.Event}: The last event.
    getLast: function() {
        return this.events[this.events.length - 1];
    },

    // Get selected event
    // @return {TreeManager.Event}: The selected event, or null if none is
    // selected.
    getSelected: function() {
        if (0 <= this.index && this.index < this.events.length) {
            return this.events[this.index];
        }
        return null;
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // EVENTS TABLE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Make cells of events table with selected nodes bold
    // @param {Node[]} selectedNodes: List of nodes for which corresponding
    // events' cells have to be selected.
    boldSelected: function(selectedNodes) {
        // Remove bold from all event cells
        let rows = this.getRows('>td.smts-bold');
        if (rows) rows.forEach(row => row.classList.remove('smts-bold'));

        // Apply bold to event cells with selected nodes
        for (let selectedNode of selectedNodes) {
            let selectedNodeNameStr = JSON.stringify(selectedNode.path);
            // node
            rows = this.getRows(`[data-node="${selectedNodeNameStr}"]`);
            if (rows) rows.forEach(row => row.children[2].classList.add('smts-bold'));
            // data-node
            rows = this.getRows(`[data-data-node="${selectedNodeNameStr}"]`);
            if (rows) rows.forEach(row => row.children[4].classList.add('smts-bold'));
        }
    },

    // Get rows of events table
    // @params {String} [optional] option: An extension to the original
    // query. If no option is provided, all rows are selected. If an option
    // is given, only rows matching the option are selected.
    // E.g.: option = '[data-node="[0,0]"]' selects only rows with
    // attribute `data-node` matching "[0.,0]".
    getRows: function(option = '') {
        let queryRows = '#smts-events-table > tbody > tr';
        return document.querySelectorAll(`${queryRows}${option}`)
    },

    // Highlight all rows with event matching one of events
    // @param {TreeManager.Events[]} events: List of events to be
    // highlighted.
    highlight: function(events) {
        // Remove highlight from all events
        let rows = this.getRows();
        if (rows) rows.forEach(row => row.classList.remove('smts-highlight'));

        // Highlight selected nodes
        for (let event of events) {
            rows = this.getRows(`[data-event="${event.id}"]`);
            if (rows) rows.forEach(row => row.classList.add('smts-highlight'));
        }
    },

    // Check if container scroll is at bottom
    isScrollBottom: function() {
        let container = document.getElementById('smts-events-table-container');
        return container.scrollTop === container.scrollHeight - container.offsetHeight;
    },

    // Check if a tab of the events container is selected
    // @param {String} tabName: the name of the tab. Has to be lowercase
    // for HTML id compatibility.
    // @return {Boolean}: `true` if the tab is active, `false` otherwise.
    isTabActive: function(tabName) {
        let tab = document.getElementById(`smts-events-navbar-${tabName}`);
        return tab ? tab.classList.contains('active') : false;
    },

    scroll: function(event) {
        let rows = this.getRows(`[data-event="${event.id}"]`);
        if (rows && rows[0]) {
            let container = document.getElementById('smts-events-table-container');
            container.scrollTop = rows[0].offsetTop;
        }
    },

    // Scroll events table container to bottom
    scrollBottom: function() {
        let container = document.getElementById('smts-events-table-container');
        container.scrollTop = container.scrollHeight - container.offsetHeight;
    },

    // Highlight next element in events table
    // Allows events navigation through up and down arrow keys while focus
    // is on events table.
    // @param {string} direction: Can be 'up' or 'down'.
    shift: function(direction) {
        let selectedEvents = this.getRows(`.smts-highlight`);

        if (selectedEvents && selectedEvents[0]) {
            let selected = selectedEvents[0];
            let sibling = direction === 'up' ?
                selected.previousElementSibling : selected.nextElementSibling;

            if (sibling) {
                selected.classList.remove('smts-highlight');
                sibling.classList.add('smts-highlight');

                // Update tree only if some time has passed, to avoid
                // generating the tree many times while holding arrows
                // to move faster in the list.
                if (this.shiftTimeout) window.clearTimeout(this.shiftTimeout);
                this.shiftTimeout = window.setTimeout(() => sibling.click(), 1500);

                // Scroll events table to right position if next selected
                // is out of visible frame.
                let container = document.getElementById('smts-events-table-container');
                if (sibling.offsetTop < container.scrollTop) {
                    container.scrollTop = sibling.offsetTop;
                } else if (container.scrollTop + container.offsetHeight <
                    sibling.offsetTop + sibling.offsetHeight) {
                    container.scrollTop =
                        sibling.offsetTop + sibling.offsetHeight - container.offsetHeight;
                }
            }
        }
    },

    // Show all rows in events table
    showAll: function() {
        let rows = this.getRows();
        if (rows) rows.forEach(row => row.classList.remove('smts-hidden'));
    },

    // Show rows of events table for which the event node matches at least
    // one of the selected nodes
    // @param {Node[]} selectedNodes: List of nodes for which corresponding
    // events have to be selected.
    showSelected: function(selectedNodes) {
        // Hide all nodes
        let rows = this.getRows();
        if (rows) rows.forEach(row => row.classList.add('smts-hidden'));

        // Show nodes that are in event.nodeName or event.data.node
        for (let selectedNode of selectedNodes) {
            let selectedNodeNameStr = JSON.stringify(selectedNode.path);
            // node
            rows = this.getRows(`[data-node="${selectedNodeNameStr}"]`);
            if (rows) rows.forEach(row => row.classList.remove('smts-hidden'));
            // data-node
            rows = this.getRows(`[data-data-node="${selectedNodeNameStr}"]`);
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
};