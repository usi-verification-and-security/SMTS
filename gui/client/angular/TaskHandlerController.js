app.controller('TaskHandler', ['$scope', '$window', '$http', 'isRealTimeDB', 'timeout', 'sharedService',
    function($scope, $window, $http, isRealTimeDB, timeout, sharedService) {

        let interval;

        $scope.$on('live-update', function() { // This is called we are in a situation of real-time analysis
            interval = setInterval(function() {
                $scope.getServerData();
            }, 3000);

        });

        $scope.getServerData = function() {
            $http({
                method: 'GET',
                url: '/getServerData'
            }).then(function successCallback(response) {
                // console.log(response.data);

                // Write which instance is being solved
                //TODO: check why the first call gives empty answer
                $scope.solvingInstance = response.data[0];
                $scope.solvingInstanceRemaining = response.data[1];

            }, function errorCallback(response) {
                // Stop intervall when the connection with the server is lost
                setInterval.cancel(interval);
                // $window.alert('An error occured!');
            });
        };

        // TODO: to prevent page redirection after posting move posting here and use "event.preventDefault();"
        $scope.increaseTimeout = function() {
            $http({
                method: 'POST',
                url: '/changeTimeout',
                data: {
                    'timeout': $("#smts-server-timeout").val(),
                    'type': "increase"
                },
            }).then(function successCallback(response) {
                event.preventDefault();

            }, function errorCallback(response) {
                // called asynchronously if an error occurs
                // or server returns response with an error status.
                $window.alert('An error occured!');
            });
        };

        $scope.decreaseTimeout = function() {
            $http({
                method: 'POST',
                url: '/changeTimeout',
                data: {
                    'timeout': $("#smts-server-timeout").val(),
                    'type': "decrease"
                },
            }).then(function successCallback(response) {

            }, function errorCallback(response) {
                // called asynchronously if an error occurs
                // or server returns response with an error status.
                $window.alert('An error occured!');
            });
        };

        $scope.stop = function() {
            $http({
                method: 'POST',
                url: '/stop'
            }).then(function successCallback(response) {

            }, function errorCallback(response) {
                // called asynchronously if an error occurs
                // or server returns response with an error status.
                $window.alert('An error occured!');
            });
        };

    }]);