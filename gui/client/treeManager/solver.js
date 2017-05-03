var TreeManager;
(function (TreeManager) {
    var Solver = (function () {
        function Solver(name) {
            this.name = name;
            this.node = [];
        }
        Solver.prototype.setData = function (data) {
            this.data = data;
        };
        return Solver;
    }());
    TreeManager.Solver = Solver;
    ;
})(TreeManager || (TreeManager = {}));
//# sourceMappingURL=solver.js.map