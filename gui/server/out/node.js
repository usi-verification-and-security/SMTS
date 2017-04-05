"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var Node = (function () {
    function Node(value) {
        this.name = value;
        console.log(value);
    }
    Node.prototype.test = function () {
        console.log("OK");
    };
    Node.prototype.getName = function () {
        console.log(this.name);
    };
    return Node;
}());
exports.Node = Node;
;
//# sourceMappingURL=node.js.map