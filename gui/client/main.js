
angular.module('myApp', ['ngFileUpload'])

    .value('currentRow', { value: 0})

    .value('rows', { value: []})

    .factory('sharedService', function($rootScope) {
    var sharedService = {};

    sharedService.broadcastItem = function() {
        $rootScope.$broadcast('handleBroadcast');
    };

    return sharedService;
})

    .controller('TreeController',['$scope','$rootScope','currentRow', 'rows','$window','$http','sharedService',function($scope,$rootScope, currentRow,rows,$window,$http,sharedService){

        // $scope.readAll = function() {
        //     $http({
        //         method : 'GET',
        //         url : 'http://localhost:3000/get'
        //     }).then(function successCallback(response) {
        //         // $rootScope.$emit("CallEventMethod", {});
        //         $window.location.reload();
        //
        //
        //     }, function errorCallback(response) {
        //         // called asynchronously if an error occurs
        //         // or server returns response with an error status.
        //         $window.alert('An error occured!');
        //     });
        // };

        $scope.readNextRow = function() {
            $http({
                method : 'GET',
                url : 'http://localhost:3000/getNext'
            }).then(function successCallback(response) {
                $window.location.reload();


            }, function errorCallback(response) {
                // called asynchronously if an error occurs
                // or server returns response with an error status.
                $window.alert('An error occured!');
            });

        };

        $scope.readPreviousRow = function() {
            $http({
                method : 'GET',
                url : 'http://localhost:3000/getPrevious'
            }).then(function successCallback(response) {
                $window.location.reload();

            }, function errorCallback(response) {
                // called asynchronously if an error occurs
                // or server returns response with an error status.
                $window.alert('An error occured!');
            });
        };

    }])

    .controller('EventController',['$scope','$rootScope','currentRow', 'rows','$window','$http', 'sharedService',function($scope,$rootScope, currentRow,rows,$window,$http,sharedService){

        $scope.$on('handleBroadcast', function() { // This is called when an instance is selected
            // $scope.message = 'ONE: ' + sharedService.message;
            // console.log($scope.message);
            $http({
                method : 'GET',
                url : 'http://localhost:3000/getEvents'
            }).then(function successCallback(response) {
                //put each entry of the response array in the table
                // console.log(response);
                // $window.location.reload();
                $scope.entries = response.data;
            }, function errorCallback(response) {
                // called asynchronously if an error occurs
                // or server returns response with an error status.
                $window.alert('An error occured!');
            });
        });

        $scope.showEvent = function(x){
            console.log(x);
        }

    }])

    .controller('SolverController',['$scope','$rootScope','currentRow', 'rows','$window','$http', 'sharedService',function($scope,$rootScope, currentRow,rows,$window,$http,sharedService){

        $scope.$on('handleBroadcast', function() { // This is called when an instance is selected
            $scope.showSolver();
        });

        $scope.showSolver = function() {
            $http({
                method : 'GET',
                url : 'http://localhost:3000/getSolvers'
            }).then(function successCallback(response) {
                console.log(response.data)
                $scope.entries = response.data;


            }, function errorCallback(response) {
                // called asynchronously if an error occurs
                // or server returns response with an error status.
                $window.alert('An error occured!');
            });
        };

        $scope.showEvent = function(x){
            console.log(x.node);
        }

    }])

    .controller('InstancesController',['$scope','$rootScope','currentRow', 'rows','$window','$http','sharedService',function($scope,$rootScope, currentRow,rows,$window,$http,sharedService){

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

        $scope.showEvent = function(x){
            // console.log(x);
            this.showTree(); // show corresponding tree
        };

        $scope.showTree = function() {
            $http({
                method : 'GET',
                url : 'http://localhost:3000/get'
            }).then(function successCallback(response) {
                sharedService.broadcastItem(); // Show events and tree


            }, function errorCallback(response) {
                // called asynchronously if an error occurs
                // or server returns response with an error status.
                $window.alert('An error occured!');
            });
        };

    }])

    .controller('ViewController',['$scope','$rootScope','currentRow', 'rows','$window','$http', 'sharedService',function($scope,$rootScope, currentRow,rows,$window,$http,sharedService){
        $scope.$on('handleBroadcast', function() { // This is called when an instance is selected
            $http({
                method : 'GET',
                url : 'http://localhost:3000/get'
            }).then(function successCallback(response) {
                getTreeJson(response.data);
            });
        });

    }]);




