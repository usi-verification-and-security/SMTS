module TreeManager {

    export class Tree {
        events: Event[] = [];
        solver: Array<[string, string]> = [];
        solvers: Solver[] = [];
        treeView: Node; // This is the tree seen in the visualization

        constructor() {
        }

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

        arrangeTree(n) {
            let treeView = new Node([], 'AND'); // The root is an 'AND'

            for (let i = 0; i <= n; ++i) {
                let event = this.events[i];
                let type = event.event;
                let data = JSON.parse(event.data);

                switch (type) {
                    case 'OR':
                    case 'AND':
                        treeView.insertNode(new Node(JSON.parse(data.node), type));
                        break;

                    case '+':
                    case '-':
                        this.updateNode(treeView, event.node, type, event.solver);
                        break;

                    case 'STATUS':
                        this.updateNode(treeView, event.node, type, data.report);
                        break;

                    case 'SOLVED':
                        this.updateNode(treeView, event.node, type, data.status);
                        this.rootSolved(treeView, data.status);
                        break;
                }
            }

            this.treeView = treeView;
        }

        updateNode(obj, node, event, data) {
            if (JSON.stringify(obj.name) == JSON.stringify(JSON.parse(node))) {
                if (event == "+") {
                    obj.solvers.push(data);
                }
                if (event == "-") {
                    var index = obj.solvers.indexOf(data);
                    if (index > -1) {
                        obj.solvers.splice(index, 1);
                    }
                }
                if (event == "STATUS") {
                    obj.status = data;
                }
                if (event == "SOLVED") {
                    obj.status = data;
                }
                return obj;
            }

            for (var i = 0; i < obj.children.length; i++) {
                var result = this.updateNode(obj.children[i], node, event, data);
                if (result) {
                    return obj;
                }
            }
        }

        // rootSolved sets the status of the root to sat or unsat
        rootSolved(obj, data) {
            obj.status = data;
            return obj;
        }

        getTreeView() {
            return this.treeView;
        }

        assignSolvers2(x: number, y: number) {

            // Reset solvers
            for (let i = 0; i < this.solvers.length; ++i) {
                this.solvers[i].node = null;
                this.solvers[i].data = null;
            }
        }

        // assignSolvers tells which solver is working on which node
        assignSolvers(x: number, y: number) {
            let i = x;

            // clear previous assignments of solvers
            for (let u = 0; u < this.solvers.length; u++) {
                this.solvers[u].node = null;
                this.solvers[u].data = null;
            }

            // assign
            for (i; i <= y; i++) {
                if (this.events[i].event == "+") {

                    for (let u = 0; u < this.solvers.length; u++) {
                        if (this.solvers[u].name == this.events[i].solver) {
                            // console.log("Assigning solver " + this.events[i].solver + " to node " + JSON.parse(this.events[i].node));
                            this.solvers[u].node = JSON.parse(this.events[i].node);
                            this.solvers[u].setData(this.events[i].data);
                        }
                    }
                }

                if (this.events[i].event == "-") {

                    for (let u = 0; u < this.solvers.length; u++) {
                        if (this.solvers[u].name == this.events[i].solver) {
                            // console.log("Removing solver " + this.events[i].solver + " from node " + JSON.parse(this.events[i].node));
                            this.solvers[u].node = null;
                            this.solvers[u].data = null;
                        }
                    }
                }

            }

        }

        //getEvents(x) returns the first x events
        getEvents(x: number) {
            if (x == this.events.length) {
                return this.events;
            }
            else {
                var events = [];
                for (var i = 0; i < x; i++) {
                    events.push(this.events[i]);
                }
                return events;
            }
        }

        initializeSolvers(array) {
            var present;
            for (var item of array) {
                present = 0;
                for (var i of this.solvers) {
                    if (i.name == item.solver) {
                        present = 1;
                    }
                }
                if (present == 0) {
                    this.solvers.push(new Solver(item.solver));
                }
            }
        }


    }

}