app.controller('SolverController', ['$scope', '$rootScope', '$window', '$http', 'sharedService', 'sharedTree', 'currentRow',
    function($scope, $rootScope, $window, $http, sharedService, sharedTree, currentRow) {

        // Trigger when an instance is selected
        $scope.$on('select-instance', function() {
            $scope.solvers = sharedTree.tree.solvers;
            sharedTree.tree.assignSolvers(0, currentRow.value);
        });

        // Trigger when an event is selected
        $scope.$on('select-event', function() {
            sharedTree.tree.assignSolvers(0, currentRow.value);
        });

        // Get execution time of solver
        // The execution time is mesured starting from the last `+ event`
        // associated to the solver and the currently selected event.
        // @param {TreeManager.Solver} solver: The solver to get the
        // execution time.
        // @return {String}: The execution time converted to stirng, or `` if
        // the solver is currently not associated with any event.
        $scope.formatExecutionTime = function(solver) {
            let event = sharedTree.tree.getEvent(currentRow.value);
            return solver.event ? `${event.ts - solver.event.ts}s` : ``;
        };

        // Update data table to contain solver information
        // @param {TreeManager.Solver} solver: the solver to be represented in
        // the data table.
        $scope.updateDataTable = function(solver) {
            smts.tables.data.update(solver.event, 'event');
            smts.tables.solvers.highlight([solver]);
        };

        // Show all solvers
        $scope.showAll = function() {
            smts.tables.solvers.showAll();
        };

        // Show only solvers related to currently selected nodes
        $scope.showSelected = function() {
            smts.tables.solvers.showSelected(sharedTree.tree.selectedNodes);
        };
    }]);