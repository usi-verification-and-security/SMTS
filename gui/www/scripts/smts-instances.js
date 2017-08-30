// Set of functions that manipulate the DOM object 'smts-instances.js-container'
smts.instances = {

    // Make cells of instances table matching one of given instances bold
    // @param {Node[]} [optional] instances: List of instances for which
    // corresponding instances' cells have to be selected. The default
    // value is an empty array, if no instances have to be selected.
    bold: function(instances = []) {
        // Remove bold from all event cells
        let rows = this.getRows('>td.smts-bold');
        if (rows) rows.forEach(row => row.classList.remove('smts-bold'));

        // Apply bold to instances cells with executing instance
        for (let instance of instances) {
            rows = this.getRows(`[data-instance="${instance.name}"]`);
            if (rows) rows.forEach(row => row.children[0].classList.add('smts-bold'));
        }
    },

    // Get selected instance name
    // @return {string}: The name of the current instance, the empty string
    // if no instance is selected.
    getSelected: function() {
        let rows = this.getRows('.smts-highlight');
        if (rows && rows[0]) {
            return rows[0].getAttribute('data-instance');
        }
        return '';
    },

    // Get rows of instances table
    // @params {String} [optional] option: An extension to the original
    // query. If no option is provided, all rows are selected. If an option
    // is given, only rows matching the option are selected.
    // E.g.: option = '[data-instance="ABCD"]' selects only rows with
    // attribute `data-instance` matching "ABCD".
    getRows: function(option = '') {
        let queryRows = '#smts-instances-table > tbody > tr';
        return document.querySelectorAll(`${queryRows}${option}`)
    },

    // Highlight all rows with instance matching one of instances
    // @param {Instance[]} instances: List of instance to be highlighted.
    highlight: function(instances) {
        // Remove highlight from all instances
        let rows = this.getRows();
        if (rows) rows.forEach(row => row.classList.remove('smts-highlight'));

        for (let instance of instances) {
            rows = this.getRows(`[data-instance="${instance.name}"]`);
            if (rows) rows.forEach(row => row.classList.add('smts-highlight'));
        }
    }
};