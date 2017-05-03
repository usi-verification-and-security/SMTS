var TreeManager;
(function (TreeManager) {
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
    TreeManager.Event = Event;
    ;
})(TreeManager || (TreeManager = {}));
//# sourceMappingURL=event.js.map