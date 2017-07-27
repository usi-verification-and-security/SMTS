smts.SMT = {

    smt: null,
    network: null,

    make: function() {

        if (!this.smt) return;

        if (this.network) this.network.destroy();

        let nodes = new vis.DataSet(this.smt.getDataSetNodes());
        let edges = new vis.DataSet(this.smt.getDataSetEdges());
        let size = this.smt.getSize();

        let container = document.getElementById('smts-content-smt-container');
        let data = {nodes: nodes, edges: edges};
        let options = {
            interaction: {
                dragNodes: true,
                hideEdgesOnDrag: true
            },
            edges:   {
                arrows: {to: {enabled: true}},
                width: 0.1,
                smooth: false,
                // physics: false
            },
            layout:  {
                // randomSeed: 2,
                improvedLayout: false
            },
            physics: {
                enabled: true,
                minVelocity: 10,
                stabilization: {enabled: true, iterations: 0},
                barnesHut: {
                    centralGravity: 0.001 * size,
                    gravitationalConstant: -100 * size
                },
            }
        };

        this.network = new vis.Network(container, data, options);

        this.network.on('stabilized', () => {
            if (this.network) this.network.stopSimulation();
        });
    },

    update: function(smt = null) {
        if (smt) this.smt = smt;
        // let smtTab = document.getElementById('smts-content-navbar-smt');
        // if (smtTab && smtTab.classList.contains('active')) {
            smts.SMT.make();
        // }
    }

};