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