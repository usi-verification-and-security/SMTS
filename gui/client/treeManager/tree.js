var TreeManager;
(function (TreeManager) {
    var Tree = (function () {
        function Tree() {
            this.events = [];
            this.solver = [];
            this.solvers = [];
        }
        Tree.prototype.createEvents = function (array) {
            var time = array[0].ts;
            var diff;
            for (var _i = 0, array_1 = array; _i < array_1.length; _i++) {
                var item = array_1[_i];
                var event = new TreeManager.Event(item);
                diff = item.ts - time;
                event.setTs(diff);
                this.events.push(event);
            }
        };
        //variable howMany tells how many rows need to be read from the db
        Tree.prototype.arrangeTree = function (howMany) {
            var treeView;
            treeView = new TreeManager.Node([], "AND");
            for (var record = 0; record < howMany; record++) {
                var parentNode = [];
                var depth = JSON.parse(this.events[record].data);
                var event = this.events[record].event;
                if (event == "OR") {
                    // var node = new Node(depth.node,"OR"); // This is for "db = prova.db"
                    var node = new TreeManager.Node(depth, "OR"); // This is for "db = global.db"
                    //Initializing (the first node seen is the root)
                    // if(!treeView){
                    //     treeView = node;
                    // }
                    //insert node in the tree
                    // else {
                    parentNode = JSON.parse(this.events[record].node);
                    treeView = this.insertNode(treeView, parentNode, node);
                    // }
                }
                if (event == "AND") {
                    var node = new TreeManager.Node(depth.node, "AND");
                    //find parent node (es. for [0,3,0,1] parent is [0,3,0])
                    for (var i = 0; i < depth.node.length - 1; ++i) {
                        parentNode.push(depth.node[i]);
                    }
                    //insert node in the tree
                    treeView = this.insertNode(treeView, parentNode, node);
                }
                if (event == "+" || event == "-") {
                    this.updateNode(treeView, this.events[record].node, event, this.events[record].solver);
                }
                var status = JSON.parse(this.events[record].data);
                if (event == "STATUS") {
                    this.updateNode(treeView, this.events[record].node, event, status.status);
                }
                if (event == "SOLVED") {
                    this.updateNode(treeView, this.events[record].node, event, status.status);
                    this.rootSolved(treeView, status.status);
                    // console.log("SOLVED! Problem "+ this.events[record].name +" is " + status.status);
                }
            }
            this.treeView = treeView;
        };
        // insertNode takes a tree object, a parent node and a child node and puts the child in parent's children array
        Tree.prototype.insertNode = function (obj, parent, child) {
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
        };
        Tree.prototype.updateNode = function (obj, node, event, data) {
            if (JSON.stringify(obj.name) == JSON.stringify(JSON.parse(node))) {
                if (event == "+") {
                    // console.log("Adding solver " + data + "to node " + node);
                    obj.solvers.push(data);
                    // console.log(obj.solvers)
                }
                if (event == "-") {
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
                return obj;
            }
            for (var i = 0; i < obj.children.length; i++) {
                var result = this.updateNode(obj.children[i], node, event, data);
                if (result) {
                    return obj;
                }
            }
        };
        // rootSolved sets the status of the root to sat or unsat
        Tree.prototype.rootSolved = function (obj, data) {
            obj.status = data;
            return obj;
        };
        Tree.prototype.getTreeView = function () {
            return this.treeView;
        };
        // assignSolvers tells which solver is working on which node
        Tree.prototype.assignSolvers = function (x, y) {
            var i = x - 1;
            // clear previous assignments of solvers
            for (var u = 0; u < this.solvers.length; u++) {
                this.solvers[u].node = null;
                this.solvers[u].data = null;
            }
            // assign
            for (i; i < y; i++) {
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
        };
        //getEvents(x) returns the first x events
        Tree.prototype.getEvents = function (x) {
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
        };
        Tree.prototype.initializeSolvers = function (array) {
            var present;
            for (var _i = 0, array_2 = array; _i < array_2.length; _i++) {
                var item = array_2[_i];
                present = 0;
                for (var _a = 0, _b = this.solvers; _a < _b.length; _a++) {
                    var i = _b[_a];
                    if (i.name == item.solver) {
                        present = 1;
                    }
                }
                if (present == 0) {
                    this.solvers.push(new TreeManager.Solver(item.solver));
                }
            }
        };
        return Tree;
    }());
    TreeManager.Tree = Tree;
})(TreeManager || (TreeManager = {}));
//# sourceMappingURL=tree.js.map