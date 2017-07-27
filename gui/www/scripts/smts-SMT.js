smts.SMT = {

    make: function(smt) {
        let nodes = new vis.DataSet(smt.getDataSetNodes());
        let edges = new vis.DataSet(smt.getDataSetEdges());

        let container = document.getElementById('smts-content-smt-container');
        let data = {nodes: nodes, edges: edges};
        let options = {
            edges:   {arrows: {to: {enabled: true}}},
            layout:  {improvedLayout: false},
            physics: {
                enabled: true,
                stabilization: {enabled: true, iterations: 0},
                barnesHut: {gravitationalConstant: -10000}
            }
        };

        let network = new vis.Network(container, data, options);

        network.on('stabilized', function() {
            network.stopSimulation();
        });
    }

};