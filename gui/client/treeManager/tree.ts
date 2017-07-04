module TreeManager {

    export class Tree {
        events: Event[] = [];
        solver: Array<[string, string]> = [];
        solvers: Solver[] = [];
        treeView: Node; // This is the tree seen in the visualization

        constructor() {
        }

        createEvents(array) {
            var time = array[0].ts;
            var diff;
            for (var item of array) {
                var event = new Event(item);
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
                        treeView = this.insertNode(treeView, JSON.parse(event.node), new Node(JSON.parse(data.node), 'OR'));
                        break;

                    case 'AND':
                        let parentNode = [];
                        let dataNode = JSON.parse(data.node);
                        for (let j = 0; j < dataNode.length - 1; ++j) {
                            parentNode.push(dataNode[j]);
                        }
                        treeView = this.insertNode(treeView, parentNode, new Node(dataNode, 'AND'));
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

        // insertNode takes a tree object, a parent node and a child node and puts the child in parent's children array
        insertNode(obj, parent, child) {
            if (JSON.stringify(obj.name) === JSON.stringify(parent)) {
                obj.addChild(child);
                return obj;
            }

            for (var i = 0; i < obj.children.length; i++) {
                var result = this.insertNode(obj.children[i], parent, child);
                if (result) {
                    return obj;
                }
            }
        }

        updateNode(obj, node, event, data) {
            // console.log(obj)
            if (JSON.stringify(obj.name) == JSON.stringify(JSON.parse(node))) {
                if (event == "+") {
                    // console.log("Adding solver " + data + "to node " + node);
                    obj.solvers.push(data);
                    // console.log(obj.solvers)
                }
                if (event == "-") {
                    // console.log("REMOVING SOLVER from node")
                    // console.log(obj.solvers)
                    var index = obj.solvers.indexOf(data);
                    // console.log("Removing solver " + data + " from node " + node);
                    if (index > -1) {
                        obj.solvers.splice(index, 1);
                    }
                    // console.log(obj.solvers)
                }
                if (event == "STATUS") {
                    // console.log("Updating status of node " + node + " from " + obj.status + " to " + data);
                    obj.status = data;
                    // console.log(obj.status);
                }
                if (event == "SOLVED") {
                    obj.status = data;
                }
                // console.log(obj)
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

        // assignSolvers tells which solver is working on which node
        assignSolvers(x: number, y: number) {
            var i = x;

            // clear previous assignments of solvers
            for (var u = 0; u < this.solvers.length; u++) {
                this.solvers[u].node = null;
                this.solvers[u].data = null;
            }

            // assign
            for (i; i <= y; i++) {
                if (this.events[i].event == "+") {

                    for (var u = 0; u < this.solvers.length; u++) {
                        if (this.solvers[u].name == this.events[i].solver) {
                            // console.log("Assigning solver " + this.events[i].solver + " to node " + JSON.parse(this.events[i].node));
                            this.solvers[u].node = JSON.parse(this.events[i].node);
                            this.solvers[u].setData(this.events[i].data);
                        }
                    }
                }

                if (this.events[i].event == "-") {

                    for (var u = 0; u < this.solvers.length; u++) {
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