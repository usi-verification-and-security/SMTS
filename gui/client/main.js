
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
        $scope.showEvent = function($event,x){
            // $event.target.parentNode.style.color= "black";
            // console.log($event.target.parentNode);
            document.getElementById('d5_2').style.color= "black";
            $event.currentTarget.style.color= "#7CFC00";
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

        };

        $scope.clickEvent = function($event,x){
            // console.log(x);
            if(x.node){
                x.node = x.node.toString(); // transform to string or it will show and array
            }
            var ppTable = prettyPrint(x);
            document.getElementById('d6_1').innerHTML = "Solver".bold();
            var item = document.getElementById('d6_2');

            if(item.childNodes[0]){
                item.replaceChild(ppTable, item.childNodes[0]); //Replace existing table
            }
            else{
                item.appendChild(ppTable);
            }

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

        $scope.clickEvent = function($event,x){
            $event.currentTarget.style.color= "#7CFC00";
            this.getTree(x); // show corresponding tree
        };
            $scope.getTree = function(x) {
                $http({
                    method : 'GET',
                    url : 'http://localhost:3000/get/' + x.name
                }).then(function successCallback(response) {
                    // Initialize tree
                    sharedTree.tree = new TreeManager.Tree();
                    sharedTree.tree.createEvents(response.data);
                    currentRow.value = response.data.length;
                    sharedTree.tree.initializeSolvers(response.data);
                    sharedTree.tree.arrangeTree(currentRow.value);

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




