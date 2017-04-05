"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var Node_1 = require("./Node");
var Event_1 = require("./Event");
var Tree = (function () {
    function Tree() {
        this.events = [];
        this.solver = [];
        this.solvers = [];
    }
    Tree.prototype.createEvents = function (array) {
        for (var _i = 0, array_1 = array; _i < array_1.length; _i++) {
            var item = array_1[_i];
            var event = new Event_1.Event(item);
            this.events.push(event);
        }
    };
    //variable howMany tells how many rows need to be read from the db
    Tree.prototype.arrangeTree = function (howMany) {
        var treeView;
        treeView = new Node_1.Node([], "AND");
        for (var record = 0; record < howMany; record++) {
            var parentNode = [];
            var depth = JSON.parse(this.events[record].data);
            var event = this.events[record].event;
            if (event == "OR") {
                // var node = new Node(depth.node,"OR"); // This is for "db = prova.db"
                var node = new Node_1.Node(depth, "OR"); // This is for "db = global.db"
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
                var node = new Node_1.Node(depth.node, "AND");
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
            if (status) {
                status.status;
            }
            if (event == "STATUS") {
                this.updateNode(treeView, this.events[record].node, event, status);
            }
            if (event == "SOLVED") {
                this.updateNode(treeView, this.events[record].node, event, status);
                this.rootSolved(treeView, status);
                console.log("SOLVED! Problem " + this.events[record].name + " is " + status);
            }
        }
        this.treeView = treeView;
        // console.log(treeView);
        // this.assignSolvers(1,howMany);  // from id=1 to id= howMany
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
                console.log("Adding solver " + data + "to node " + node);
                obj.solvers.push(data);
                console.log(obj.solvers);
            }
            if (event == "-") {
                var index = obj.solvers.indexOf(data);
                console.log("Removing solver " + data + "from node " + node);
                if (index > -1) {
                    obj.solvers.splice(index, 1);
                }
                console.log(obj.solvers);
            }
            if (event == "STATUS") {
                console.log("Updating status of node " + node + " from " + obj.status + " to " + data);
                obj.status = data;
                console.log(obj.status);
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
    Tree.prototype.getSolvers = function () {
        return null;
    };
    // assignSolvers saves the solvers working in a given period of time (from event id = x to event id = y)
    Tree.prototype.assignSolvers = function (x, y) {
        // let foo:[{ [solver:string] : {node: string} }] = [];
        // for(var z= 0; z< this.solvers.length;z++){
        //
        // }
        // var i = x -1;
        // for (i ; i < y; i++) {
        //     if(this.events[i].event == "AND" ){
        //         for(var u=0; u< this.solvers.length;u++){
        //             if(this.solvers[u][0] == this.events[i].solver){
        //                 console.log("inside");
        //                 // this.solvers[u][1] = JSON.parse(this.events[i].node);
        //             }
        //         }
        //             // if(this.solvers[u].solver == this.events[i].solver){
        //             //     console.log("inside");
        //             //     solver[u][1] = this.events[i].node;
        //             // }
        //
        //         }
        //     }
        // todo: change how to remove, this version is not working because index is not found
        // if(this.events[i].event == "-"){
        //     // var index = solver.indexOf([this.events[i].node, this.events[i].solver]);
        //     // console.log([this.events[i].node,this.events[i].solver]);
        //     // console.log(solver[index]);
        //     // console.log(index);
        //     // if (index > -1) {
        //     //     solver.splice(index, 1);
        //     // }
        //     for(var u=0; u< solver.length;u++){
        //         if(solver[u][0]==this.events[i].node){
        //
        //         }
        //
        //     }
        //
        //
        // }
        // }
        // console.log("My solver: ");
        // console.log(solver);
        // this.solver = solver;
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
    return Tree;
}());
exports.Tree = Tree;
;
//# sourceMappingURL=tree.js.map