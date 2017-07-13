app.controller('SolverController', ['$scope', '$rootScope', 'currentRow', 'sharedTree', '$window', '$http', 'sharedService',
    function ($scope, $rootScope, currentRow, sharedTree, $window, $http, sharedService) {
        $scope.$on('handleBroadcast', function () { // This is called when an instance is selected
            $scope.showSolver();
        });

        $scope.$on('handleBroadcast2', function () { // This is called when an event is selected and the new tree shows up
            $scope.showSolver();
        });

        $scope.showSolver = function () {
            $('#smts-solver-container table tr').removeClass("smts-highlight");
            sharedTree.tree.assignSolvers(0, currentRow.value);
            $scope.solvers = sharedTree.tree.solvers;
        };

        $scope.clickEvent = function ($event, solver) {
            // Make solver object to show in table
            let ppSolver = {};
            ppSolver.name = solver.name;
            ppSolver.node = JSON.stringify(solver.node); // Transform to string or it will show and array
            ppSolver.data = solver.data;
            let ppTable = prettyPrint(ppSolver);

            // Update data table in DOM
            document.getElementById('smts-data-container-title').innerHTML = "SOLVER".bold();
            let dataContent = document.getElementById('smts-data-container-content');
            dataContent.innerHTML = '';
            dataContent.appendChild(ppTable);
        }

        // Show all solvers
        $scope.showAll = function() {
            let rows = document.querySelectorAll('#smts-solver-table > tbody > tr');
            for (let row of rows) {
                row.classList.remove('smts-hidden');
            }
        };

        // Show only solvers related to selected nodes
        $scope.showSelected = function() {
            let rows = document.querySelectorAll('#smts-solver-table > tbody > tr');
            for (let row of rows) {
                let nodeName = row.children[1].innerHTML;
                if (!nodeName || isNotNodeInNodes({name: JSON.parse(nodeName)}, sharedTree.tree.selectedNodes)) {
                    row.classList.add('smts-hidden');
                }
            }
        };
    }]);