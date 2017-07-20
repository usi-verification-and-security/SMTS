module TreeManager {

    export class Tree {
        root: Node;                           // Tree seen in the visualization
        events: Event[]                 = []; // All events
        solvers: Solver[]               = []; // All existing solvers
        selectedNodes: Node[]           = []; // List of selected nodes


        //
        constructor() {
        }


        // Populate the tree up until the n-th event (included)
        arrangeTree(n) {
            this.root = new Node([], 'AND'); // The root is an 'AND'

            for (let i = 0; i <= n; ++i) {
                let event = this.events[i];
                this.root.update(event);
            }

            this.updateSelectedNodes(n);
        }

        //
        createEvents(array) {
            let time = array[0].ts;
            let diff;
            for (let item of array) {
                let event = new Event(item);
                diff = item.ts - time;
                event.setTs(diff);
                this.events.push(event);
            }
        }

        // Returns the first `n` events
        getEvents(n: number) {
            if (n == this.events.length) {
                return this.events;
            }
            return this.events.slice(0, n + 1);
        }

        getEvent(n: number) {
            return this.events[n];
        }

        // Set selected node
        setSelectedNodes(nodes) {
            this.selectedNodes.length = 0;
            for (let node of nodes) {
                this.selectedNodes.push(node);
            }
        }

        // Update selected nodes
        updateSelectedNodes(n) {
            this.selectedNodes.length = 0; // Clear selected nodes
            let event = this.events[n];
            this.selectedNodes.push(this.root.getNode(event.node));
            if (event.data && event.data.node) {
                this.selectedNodes.push(this.root.getNode(JSON.parse(event.data.node)));
            }
        }

        //
        assignSolvers(begin: number, end: number) {
            // Reset solvers
            this.solvers.forEach(solver => solver.update(null));

            for (let i = begin; i <= end; ++i) {
                let event = this.events[i];
                switch (event.event) {
                    case '+':
                        this.solvers.forEach(function (solver) {
                            if (solver.name === event.solver) {
                                solver.update(event);
                            }
                        });
                        break;

                    case '-':
                        this.solvers.forEach(function (solver) {
                            if (solver.name === event.solver) {
                                solver.update(null);
                            }
                        });
                        break;
                }
            }
        }


        // Insert solvers in `this.solvers`, only if the solvers are not already present
        initializeSolvers(nodes) {
            let isPresent;
            for (let node of nodes) {
                if (!node.solver) {
                    continue;
                }
                isPresent = false;
                for (let solver of this.solvers) {
                    if (solver.name == node.solver) {
                        isPresent = true;
                    }
                }
                if (!isPresent) {
                    this.solvers.push(new Solver(node.solver));
                }
            }
        }
    }
}