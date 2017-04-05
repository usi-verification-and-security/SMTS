///<reference path='../out/tree.d.ts'/>
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var Node_1 = require("./Node");
var Tree = (function () {
    function Tree(array) {
        this.node = [];
        this.solver = [];
        this.createNodes(array);
        // this.test();
    }
    Tree.prototype.test = function () {
        // console.log(this.node);
        console.log(this.node[0]);
        // console.log(this.node[0].getName());
    };
    Tree.prototype.createNodes = function (array) {
        for (var _i = 0, array_1 = array; _i < array_1.length; _i++) {
            var item = array_1[_i];
            var nodeEntry = new Node_1.Node(item);
            this.node.push(nodeEntry);
        }
    };
    return Tree;
}());
exports.Tree = Tree;
;
//# sourceMappingURL=tree.js.map