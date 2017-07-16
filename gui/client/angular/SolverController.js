app.controller('SolverController', ['$scope', '$rootScope', 'currentRow', 'sharedTree', '$window', '$http', 'sharedService',
    function ($scope, $rootScope, currentRow, sharedTree, $window, $http, sharedService) {

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
            tables.data.update(solver, 'solver');
            tables.solvers.highlight([solver]);
        };

        // Show all solvers
        $scope.showAll = function() {
            tables.solvers.showAll();
        };

        // Show only solvers related to currently selected nodes
        $scope.showSelected = function() {
            tables.solvers.showSelected(sharedTree.tree.selectedNodes);
        };
    }]);