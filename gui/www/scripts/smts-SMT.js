smts.SMT = {

    smt: null,
    network: null,

    nodeBool: {
        color: {
            border: '#682b2b',
            background: '#913b3b',
            highlight: {
                border: '#913b3b',
                background: '#e05e5e',
            }
        },
        shape: 'diamond'
    },

    nodeOther: {
        color: {
            border: '#019e01',
            background: '#32fc32',
            highlight: {
                border: '#32fc32',
                background: '#8dfc8d',
            }
        },
        shape: 'circle'
    },

    // Get a data set for the nodes representation with vis.js
    // @return {object[]}: An array containing the node objects.
    getDataSetNodes() {
        let dataNodes = [];
        for (let key in this.smt.nodes) {
            let nodes = this.smt.nodes[key];
            for (let node of nodes) {
                let ret = this.smt.getType(node.name);
                let nodeType =  ret === 'Bool' ? this.nodeBool : this.nodeOther;
                dataNodes.push({
                    id:    `${key}-${node.pos}`,
                    label: node.name,
                    color: nodeType.color,
                    shape: nodeType.shape
                });
            }
        }
        return dataNodes;
    },

    // Get a data set for the edges representation with vis.js
    // @return {object[]}: An array containing the edge objects.
    getDataSetEdges() {
        let dataEdges = [];
        for (let key in this.smt.nodes) {
            let nodes = this.smt.nodes[key];
            for (let node of nodes) {
                let nodeId = `${key}-${node.pos}`;
                for (let arg of node.args) {
                    dataEdges.push({
                        from: nodeId,
                        to:   `${arg.name}-${arg.pos}`
                    });
                }
            }
        }
        return dataEdges;
    },

    make: function() {

        if (!this.smt) return;

        if (this.network) this.network.destroy();

        let nodes = new vis.DataSet(this.getDataSetNodes());
        let edges = new vis.DataSet(this.getDataSetEdges());
        let size = this.smt.getSize();

        let container = document.getElementById('smts-content-cnf-container');
        let data = {nodes: nodes, edges: edges};
        let options = {
            interaction: {
                dragNodes: true,
                hideEdgesOnDrag: true
            },
            nodes: {
                // physics: false
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

        // this.network = new vis.Network(container, data, options);
        //
        // this.network.on('stabilized', () => {
        //     if (this.network) this.network.stopSimulation();
        // });
    },

    update: function(smt = null) {
        if (smt) this.smt = smt;
        // let smtTab = document.getElementById('smts-content-navbar-smt');
        // if (smtTab && smtTab.classList.contains('active')) {
            smts.SMT.make();
        // }
    }

};