"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var Node = (function () {
    function Node(name, type) {
        this.children = [];
        this.solvers = []; // solvers working on it
        this.name = name;
        this.type = type;
        this.status = "unknown";
    }
    Node.prototype.test = function () {
    };
    Node.prototype.getName = function () {
        console.log(this.name);
    };
    Node.prototype.addChild = function (child) {
        this.children.push(child);
    };
    return Node;
}());
exports.Node = Node;
;
//# sourceMappingURL=node.js.map