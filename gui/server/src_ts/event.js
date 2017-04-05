"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var Event = (function () {
    function Event(value) {
        this.children = [];
        this.id = value.id;
        this.ts = value.ts;
        this.name = value.name;
        this.node = value.node;
        this.event = value.event;
        this.solver = value.solver;
        this.data = value.data;
    }
    return Event;
}());
exports.Event = Event;
;
//# sourceMappingURL=event.js.map