
angular.module('myApp', ['ngFileUpload'])

    .value('currentRow', { value: 0})

    .value('rows', { value: []})

    .factory('sharedService', function($rootScope) {
    var sharedService = {};

    sharedService.broadcastItem = function() {
        $rootScope.$broadcast('handleBroadcast');
    };

    sharedService.broadcastItem2 = function() {
        $rootScope.$broadcast('handleBroadcast2');
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

        // $scope.readNextRow = function() {
        //     $http({
        //         method : 'GET',
        //         url : 'http://localhost:3000/getNext'
        //     }).then(function successCallback(response) {
        //         $window.location.reload();
        //
        //
        //     }, function errorCallback(response) {
        //         // called asynchronously if an error occurs
        //         // or server returns response with an error status.
        //         $window.alert('An error occured!');
        //     });
        // };
        //
        // $scope.readPreviousRow = function() {
        //     $http({
        //         method : 'GET',
        //         url : 'http://localhost:3000/getPrevious'
        //     }).then(function successCallback(response) {
        //         $window.location.reload();
        //
        //     }, function errorCallback(response) {
        //         // called asynchronously if an error occurs
        //         // or server returns response with an error status.
        //         $window.alert('An error occured!');
        //     });
        // };

    }])

    .controller('EventController',['$scope','$rootScope','currentRow', 'rows','$window','$http', 'sharedService',function($scope,$rootScope, currentRow,rows,$window,$http,sharedService){

        $scope.$on('handleBroadcast', function() { // This is called when an instance is selected
            $http({
                method : 'GET',
                url : 'http://localhost:3000/getEvents'
            }).then(function successCallback(response) {
                $scope.entries = response.data;
            }, function errorCallback(response) {
                // called asynchronously if an error occurs
                // or server returns response with an error status.
                $window.alert('An error occured!');
            });
        });

        // Show tree up to clicked event
        $scope.showEvent = function(x){
            $http({
                method : 'GET',
                url : 'http://localhost:3000/getNext/' + x.id
            }).then(function successCallback(response) {
                // sharedService.broadcastItem2(); // Show tree and solvers
                getTreeJson(response.data); // Show new tree
                sharedService.broadcastItem2();
            }, function errorCallback(response) {
                // called asynchronously if an error occurs
                // or server returns response with an error status.
                $window.alert('An error occured!');
            });
        }

    }])

    .controller('SolverController',['$scope','$rootScope','currentRow', 'rows','$window','$http', 'sharedService',function($scope,$rootScope, currentRow,rows,$window,$http,sharedService){

        $scope.$on('handleBroadcast', function() { // This is called when an instance is selected
            $scope.showSolver();
        });
        $scope.$on('handleBroadcast2', function() { // This is called when an instance is selected
            $scope.showSolver();
        });

        $scope.showSolver = function() {
            $http({
                method : 'GET',
                url : 'http://localhost:3000/getSolvers'
            }).then(function successCallback(response) {
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
                sharedService.broadcastItem(); // Show events, tree and solvers


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
        // $scope.$on('handleBroadcast2', function() { // This is called when an instance is selected
        //     $http({
        //         method : 'GET',
        //         url : 'http://localhost:3000/get'
        //     }).then(function successCallback(response) {
        //         getTreeJson(response.data);
        //     });
        // });

    }]);




