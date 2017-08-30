app.controller('SolverController', ['$scope', '$rootScope', '$window', '$http', 'sharedService',
    function($scope, $rootScope, $window, $http, sharedService, sharedTree) {

        // Trigger when an instance is selected
        $scope.$on('select-instance', function() {
            $scope.solvers = smts.tree.tree.solvers;
            smts.tree.tree.assignSolvers(0, smts.events.index);
        });

        // Trigger when an event is selected
        $scope.$on('select-event', function() {
            smts.tree.tree.assignSolvers(0, smts.events.index);
        });

        // Get learnts clauses of currently selected solver
        // @params {TreeManager.Solver} solver: Solver from which to retrieve
        // the CNF.
        $scope.getLearnts = function(evt, solver) {
            evt.stopPropagation();
            let instanceName = smts.instances.getSelected();
            let nodePath = JSON.stringify(smts.tree.getSelectedNodes()[0].name);
            let solverName = solver.name;
            smts.cnf.load(instanceName, nodePath, solverName);
        };

        // Get execution time of solver
        // The execution time is mesured starting from the last `+ event`
        // associated to the solver and the currently selected event.
        // @param {TreeManager.Solver} solver: The solver to get the
        // execution time.
        // @return {String}: The execution time converted to stirng, or `` if
        // the solver is currently not associated with any event.
        $scope.formatExecutionTime = function(solver) {
            let event = smts.tree.tree.getEvent(smts.events.index);
            return solver.event ? `${event.ts - solver.event.ts}s` : ``;
        };

        // Update data table to contain solver information
        // @param {TreeManager.Solver} solver: the solver to be represented in
        // the data table.
        $scope.updateDataTable = function(solver) {
            smts.data.update(solver.event, 'event');
            smts.solvers.highlight([solver]);
        };

        // Show all solvers
        $scope.showAll = function() {
            smts.solvers.showAll();
        };

        // Show only solvers related to currently selected nodes
        $scope.showSelected = function() {
            smts.solvers.showSelected(smts.tree.tree.selectedNodes);
        };
    }]);