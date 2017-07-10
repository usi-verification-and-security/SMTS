module TreeManager {

    export class Tree {
        events: Event[] = [];                 // All events
        solver: Array<[string, string]> = []; // ???
        solvers: Solver[] = [];               // All existing solvers
        root: Node;                           // Tree seen in the visualization


        //
        constructor() {
        }


        // Populate the tree up until the n-th event (included)
        arrangeTree(n) {
            let root = new Node([], 'AND'); // The root is an 'AND'

            for (let i = 0; i <= n; ++i) {
                root.update(this.events[i]);
            }

            this.root = root;
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


        // Get name of selected node(s)
        getSelectedNodeNames(n) {
            let event = this.events[n];
            let selectedNodeNames = [];
            selectedNodeNames.push(event.node);
            if (event.data && event.data.node) {
                selectedNodeNames.push(JSON.parse(event.data.node));
            }
            return selectedNodeNames;
        }


        // Get `this.root`
        getRoot() {
            return this.root;
        }


        //
        assignSolvers(begin: number, end: number) {
            // Reset solvers
            for (let i = 0; i < this.solvers.length; ++i) {
                this.solvers[i].setNode(null);
                this.solvers[i].setData(null);
            }

            for (let i = begin; i <= end; ++i) {
                let event = this.events[i];
                switch (event.event) {
                    case '+':
                        for (let j = 0; j < this.solvers.length; ++j) {
                            let solver = this.solvers[j];
                            if (solver.name === event.solver) {
                                solver.setNode(event.node);
                                solver.setData(event.data);
                            }
                        }
                        break;

                    case '-':
                        for (let j = 0; j < this.solvers.length; ++j) {
                            let solver = this.solvers[j];
                            if (solver.name === event.solver) {
                                solver.setNode(null);
                                solver.setData(null);
                            }
                        }
                        break;
                }
            }
        }


        // Insert solvers in `this.solvers`, only if the solvers are not already present
        initializeSolvers(array) {
            let isPresent;
            for (let item of array) {
                isPresent = false;
                for (let solver of this.solvers) {
                    if (solver.name == item.solver) {
                        isPresent = true;
                    }
                }
                if (!isPresent) {
                    this.solvers.push(new Solver(item.solver));
                }
            }
        }
    }
}