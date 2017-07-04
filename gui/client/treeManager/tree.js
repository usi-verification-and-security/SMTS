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
        Tree.prototype.arrangeTree = function (n) {
            var treeView = new TreeManager.Node([], 'AND'); // The root is an 'AND'
            for (var i = 0; i <= n; ++i) {
                var event_1 = this.events[i];
                var type = event_1.event;
                var data = JSON.parse(event_1.data);
                console.log(treeView);
                switch (type) {
                    case 'OR':
                        treeView = this.insertNode(treeView, JSON.parse(event_1.node), new TreeManager.Node(JSON.parse(data.node), 'OR'));
                        break;
                    case 'AND':
                        var parentNode = [];
                        var dataNode = JSON.parse(data.node);
                        for (var j = 0; j < dataNode.length - 1; ++j) {
                            parentNode.push(dataNode[j]);
                        }
                        //console.log(treeView);
                        treeView = this.insertNode(treeView, parentNode, new TreeManager.Node(dataNode, 'AND'));
                        break;
                    case '+':
                    case '-':
                        this.updateNode(treeView, event_1.node, type, event_1.solver);
                        break;
                    case 'STATUS':
                        this.updateNode(treeView, event_1.node, type, data.report);
                        break;
                    case 'SOLVED':
                        this.updateNode(treeView, event_1.node, type, data.status);
                        this.rootSolved(treeView, data.status);
                        break;
                }
            }
            this.treeView = treeView;
        };
        //variable howMany tells how many rows need to be read from the db
        Tree.prototype.arrangeTree1 = function (howMany) {
            var treeView;
            treeView = new TreeManager.Node([], "AND");
            for (var record = 0; record <= howMany; record++) {
                var parentNode = [];
                //console.log(this.events[record]);
                var depth = JSON.parse(this.events[record].data);
                var event = this.events[record].event;
                if (event == "OR") {
                    var node = new TreeManager.Node(JSON.parse(depth.node), "OR"); // This is for "db = prova.db" and the big database
                    // var node = new Node(depth.node,"OR"); // This is for "db = opensmt.db"
                    parentNode = JSON.parse(this.events[record].node);
                    treeView = this.insertNode(treeView, parentNode, node);
                }
                if (event == "AND") {
                    var node = new TreeManager.Node(JSON.parse(depth.node), "AND");
                    // var node = new Node(depth.node, "AND");// This is for "db = opensmt.db"
                    // find parent node (es. for [0,3,0,1] parent is [0,3,0])
                    // for (var i = 0; i < depth.node.length - 1; ++i) {// This is for "db = opensmt.db"
                    for (var i = 0; i < JSON.parse(depth.node).length - 1; ++i) {
                        parentNode.push(JSON.parse(depth.node)[i]);
                        // parentNode.push(depth.node[i]); // This is for "db = opensmt.db"
                    }
                    //insert node in the tree
                    treeView = this.insertNode(treeView, parentNode, node);
                }
                if (event == "+" || event == "-") {
                    // console.log(treeView)
                    this.updateNode(treeView, this.events[record].node, event, this.events[record].solver);
                }
                var status = JSON.parse(this.events[record].data);
                if (event == "STATUS") {
                    this.updateNode(treeView, this.events[record].node, event, status.report);
                }
                if (event == "SOLVED") {
                    this.updateNode(treeView, this.events[record].node, event, status.status);
                    this.rootSolved(treeView, status.status);
                    // console.log("SOLVED! Problem "+ this.events[record].name +" is " + status.status);
                }
                // console.log(treeView)
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
