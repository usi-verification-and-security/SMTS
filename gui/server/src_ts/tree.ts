
export { Tree };
import { Node } from "./Node";
import { Event } from "./Event";
import { Solver } from "./Solver";

class Tree {
    events: Event[] = [];
    solver: Array< [ string, string ]> = [];
    solvers: Solver[] = [];
    treeView: Node ; // This is the tree seen in the visualization

    constructor() {
    }

    createEvents(array: Array<Object>){
        for (var item of array) {
            var event = new Event(item);
            this.events.push(event);
        }
    }

    //variable howMany tells how many rows need to be read from the db
    arrangeTree(howMany){
        var treeView;

        treeView = new Node([],"AND");

        for (var record = 0; record < howMany; record++) {
            var parentNode = [];

            var depth = JSON.parse(this.events[record].data);
            var event = this.events[record].event;

            if(event == "OR" ){
                // var node = new Node(depth.node,"OR"); // This is for "db = prova.db"
                var node = new Node(depth,"OR"); // This is for "db = global.db"

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

            if(event == "AND"){
                var node = new Node(depth.node, "AND");


                //find parent node (es. for [0,3,0,1] parent is [0,3,0])
                for (var i = 0; i < depth.node.length -1; ++i){
                    parentNode.push(depth.node[i]);
                }

                //insert node in the tree
                treeView = this.insertNode(treeView, parentNode, node);
            }

            if(event == "+" || event == "-" ){
                this.updateNode(treeView,this.events[record].node, event, this.events[record].solver);
            }

            var status = JSON.parse(this.events[record].data);

            if(event == "STATUS"){
                this.updateNode(treeView,this.events[record].node, event, status.status);
            }
            if(event == "SOLVED"){
                this.updateNode(treeView,this.events[record].node, event, status.status);
                this.rootSolved(treeView, status.status);
                console.log("SOLVED! Problem "+ this.events[record].name +" is " + status.status);
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

        for (var i = 0; i < obj.children.length; i ++) {
            var result = this.insertNode(obj.children[i], parent, child);
            if(result){
                return obj;
            }
        }
    }

    updateNode(obj, node, event, data){
        if (JSON.stringify(obj.name) == JSON.stringify(JSON.parse(node))) {
            if(event == "+"){
                console.log("Adding solver " + data + "to node " + node);
                obj.solvers.push(data);
                console.log(obj.solvers)
            }
            if(event == "-"){
                var index = obj.solvers.indexOf(data);
                    console.log("Removing solver " + data + " from node " + node);
                    if (index > -1) {
                        obj.solvers.splice(index, 1);
                    }
                console.log(obj.solvers)
            }
            if(event == "STATUS"){
                console.log("Updating status of node " + node + " from " + obj.status + " to " + data);
                obj.status = data;
                console.log(obj.status);
            }
            if(event == "SOLVED"){
                obj.status = data;
            }
            return obj;
        }

        for (var i = 0; i < obj.children.length; i ++) {
            var result = this.updateNode(obj.children[i], node, event, data);
            if(result){
                return obj;
            }
        }
    }

    // rootSolved sets the status of the root to sat or unsat
    rootSolved(obj,data){
        obj.status = data;
        return obj;
    }

    getTreeView(){
        return this.treeView;
    }

    getSolvers(){
        return this.solvers;
    }

    // assignSolvers tells which solver is working on which node
    assignSolvers(x: number, y:number){
        var i = x -1;
        for (i ; i < y; i++) {

            if(this.events[i].event == "+" ){

                for(var u=0; u< this.solvers.length;u++){
                    if(this.solvers[u].name == this.events[i].solver){
                        // console.log("Assigning solver " + this.events[i].solver + " to node " + JSON.parse(this.events[i].node));
                        this.solvers[u].node = JSON.parse(this.events[i].node);
                    }
                }
            }

            if(this.events[i].event == "-" ){

                for(var u=0; u< this.solvers.length;u++){
                    if(this.solvers[u].name == this.events[i].solver){
                        // console.log("Removing solver " + this.events[i].solver + " from node " + JSON.parse(this.events[i].node));
                        this.solvers[u].node = null;
                    }
                }
            }

        }

    }

    //getEvents(x) returns the first x events
    getEvents(x: number){
        if(x == this.events.length){
            return this.events;
        }
        else{
            var events = [];
            for(var i =0 ; i < x ; i++){
                events.push(this.events[i]);
            }
            return events;
        }

    }

};