module TreeManager {

    export class Tree {
        root:          Node;          // Tree seen in the visualization
        events:        Event[]  = []; // All events
        solvers:       Solver[] = []; // All existing solvers
        selectedNodes: Node[]   = []; // List of selected nodes

        // Constructor
        // @param {any[]} events: List of events needed to build the tree.
        constructor(events: any[]) {
            this.initializeEvents(events);
            this.initializeSolvers(events);
        }

        // Initialize events of the tree
        // @param {any[]} events: List of events needed to build the tree.
        initializeEvents(events: any[]) : void {
            let startTime: number = events[0].ts;
            for (let event of events) {
                this.events.push(new Event(event, startTime));
            }
        }

        // Initialize solvers of the tree
        // @param {any[]} events: List of events from which the solvers are
        // taken.
        initializeSolvers(nodes) : void {
            for (let node of nodes) {
                if (!node.solver) {
                    continue;
                }
                // Insert solver in `this.solvers`, only if the solver is not
                // already present.
                if (!this.solvers.some(solver => solver.name === node.solver)) {
                    this.solvers.push(new Solver(node.solver));
                }
            }
        }

        // Build the tree nodes up until the n-th event (included)
        // @param {number} n: Maximum index of the event to be taken into
        // account to generate the nodes.
        resize(n: number) : void {
            this.root = new Node([], 'AND'); // The root is always an 'AND'

            for (let i = 0; i <= n; ++i) {
                this.root.update(this.events[i]);
            }

            this.updateSelectedNodes(n);
        }

        // Get the first `n` events (as a copy of the original)
        // @return {Event[]}: A copy of the first n events.
        getEvents(n: number) : Event[] {
            return this.events.slice(0, n + 1);
        }

        // Get the i-th element
        // @return {Event}: The i-th event.
        getEvent(i: number) : Event {
            return this.events[i];
        }

        // Set selected nodes
        // @param {Node[]} nodes: The new selected nodes.
        setSelectedNodes(nodes: Node[]) : void {
            this.selectedNodes.length = 0;
            for (let node of nodes) {
                this.selectedNodes.push(node);
            }
        }

        // Set selected nodes associated to a particular event
        // @param {number} i: The index of the event. The new selected nodes
        // are the nodes appearing in said event.
        updateSelectedNodes(i: number) : void {
            this.selectedNodes.length = 0; // Clear selected nodes
            let event = this.events[i];
            this.selectedNodes.push(this.root.getNode(event.node));
            if (event.data && event.data.node) {
                this.selectedNodes.push(this.root.getNode(JSON.parse(event.data.node)));
            }
        }

        // Assign solvers to events in range `begin`-`end`
        // @param {number} begin: Index of the first event.
        // @param {number} end: Index of the last event.
        assignSolvers(begin: number, end: number) {
            // Reset solvers
            this.solvers.forEach(solver => solver.update(null));

            for (let i = begin; i <= end; ++i) {
                let event = this.events[i];
                switch (event.type) {
                    case '+':
                        this.solvers.forEach(function(solver) {
                            if (solver.name === event.solver) {
                                solver.update(event);
                            }
                        });
                        break;

                    case '-':
                        this.solvers.forEach(function(solver) {
                            if (solver.name === event.solver) {
                                solver.update(null);
                            }
                        });
                        break;
                }
            }
        }
    }
}