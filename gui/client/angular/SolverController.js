app.controller('SolverController', ['$scope', '$rootScope', 'currentRow', 'sharedTree', '$window', '$http', 'sharedService',
    function ($scope, $rootScope, currentRow, sharedTree, $window, $http, sharedService) {
        $scope.$on('select-instance', function () { // This is called when an instance is selected
            $scope.loadSolvers();
        });

        $scope.$on('select-event', function () {
            $scope.loadSolvers();
        });

        $scope.loadSolvers = function () {
            sharedTree.tree.assignSolvers(0, currentRow.value);
            $scope.solvers = sharedTree.tree.solvers;
        };

        $scope.updateDataTable = function (solver) {
            // Make solver object to show in data table
            let dataTableSolver = {};
            dataTableSolver.name = solver.name;
            dataTableSolver.node = JSON.stringify(solver.node); // Transform to string or it will show and array
            dataTableSolver.data = solver.data;
            let dataTable = prettyPrint(dataTableSolver);

            // Update data table in DOM
            document.getElementById('smts-data-title').innerHTML = 'Solver';
            let dataTableContainer = document.getElementById('smts-data-table-container');
            dataTableContainer.innerHTML = '';
            dataTableContainer.appendChild(dataTable);

            // Highlight solver
            tables.solvers.highlight([solver]);
        };

        // Show all solvers
        $scope.showAll = function() {
            tables.solvers.showAll();
        };

        // Show only solvers related to selected nodes
        $scope.showSelected = function() {
            tables.solvers.showSelected(sharedTree.tree.selectedNodes);
        };
    }]);