
angular.module('myApp', ['ngFileUpload'])

    // Variable used to keep track how many rows of the db needs to be read
    .value('currentRow', { value: 0})

    .factory('sharedService', function($rootScope) {
    var sharedService = {};

    // This is used to show solvers, tree and events when an instance is selected
    sharedService.broadcastItem = function() {
        $rootScope.$broadcast('handleBroadcast');
    };

    // This is used to show in solver view solver's new status when an event is clicked and a new tree is displayed
    sharedService.broadcastItem2 = function() {
        $rootScope.$broadcast('handleBroadcast2');
    };

    return sharedService;
    })

    //Tree
    .factory('sharedTree', function() {
        var tree = new TreeManager.Tree();
        return tree;

    })

    .controller('TreeController',['$scope','$rootScope','currentRow','$window','$http','sharedService',function($scope,$rootScope, currentRow,$window,$http,sharedService){

    }])

    .controller('EventController',['$scope','$rootScope','currentRow','sharedTree','$window','$http', 'sharedService',function($scope,$rootScope, currentRow,sharedTree,$window,$http,sharedService){

        $scope.$on('handleBroadcast', function() { // This is called when an instance is selected
            var eventEntries = sharedTree.tree.getEvents(currentRow.value);
            $scope.entries = eventEntries;

        });

        // Show tree up to clicked event
        $scope.showEvent = function(x){
            currentRow.value = x.id;
            sharedTree.tree.arrangeTree(currentRow.value);
            var treeView = sharedTree.tree.getTreeView();
            getTreeJson(treeView);
            sharedService.broadcastItem2();
        }

    }])

    .controller('SolverController',['$scope','$rootScope','currentRow','sharedTree','$window','$http', 'sharedService',function($scope,$rootScope, currentRow,sharedTree,$window,$http,sharedService){
        $scope.$on('handleBroadcast', function() { // This is called when an instance is selected
            $scope.showSolver();
        });
        $scope.$on('handleBroadcast2', function() { // This is called when an event is selected and the new tree shows up
            $scope.showSolver();
        });

        $scope.showSolver = function() {
            sharedTree.tree.assignSolvers(1,currentRow.value);
            $scope.entries = sharedTree.tree.solvers;

            // var ppTable = prettyPrint(sharedTree.tree.solvers);
            // var item = document.getElementById('d4_2');
            // if(item.childNodes[0]){
            //     item.replaceChild(ppTable, item.childNodes[0]); //Replace existing table
            // }
            // else{
            //     item.appendChild(ppTable);
            // }
        };

        $scope.showEvent = function(x){
            console.log(x.node);
        }

    }])

    .controller('InstancesController',['$scope','$rootScope','currentRow','sharedTree','$window','$http','sharedService',function($scope,$rootScope, currentRow,sharedTree,$window,$http,sharedService){

        $scope.load = function() {
            $http({
                method : 'GET',
                url : 'http://localhost:3000/getInstances'
            }).then(function successCallback(response) {
                //put each entry of the response array in the table
                $scope.entries = response.data;

            }, function errorCallback(response) {
                // called asynchronously if an error occurs
                // or server returns response with an error status.
                $window.alert('An error occured!');
            });
        };

        $scope.clickEvent = function(x){
            // this.getTree(); // show corresponding tree
            this.getTree(x);
        };
            $scope.getTree = function(x) {
            // console.log(x.name);
                $http({
                    method : 'GET',
                    url : 'http://localhost:3000/get/' + x.name
                }).then(function successCallback(response) {
                    // Initialize tree
                    sharedTree.tree = new TreeManager.Tree();
                    sharedTree.tree.createEvents(response.data);
                    currentRow.value = response.data.length;
                    sharedTree.tree.arrangeTree(currentRow.value);
                    sharedTree.tree.initializeSolvers(response.data);

                    sharedService.broadcastItem(); // Show events, tree and solvers


                }, function errorCallback(response) {
                    // called asynchronously if an error occurs
                    // or server returns response with an error status.
                    $window.alert('An error occured!');
                });


            };

    }])

    .controller('ViewController',['$scope','$rootScope','currentRow','sharedTree','$window','$http', 'sharedService',function($scope,$rootScope, currentRow,sharedTree,$window,$http,sharedService){
        $scope.$on('handleBroadcast', function() { // This is called when an instance is selected
            var treeView = sharedTree.tree.getTreeView();
            getTreeJson(treeView);
        });

    }]);




