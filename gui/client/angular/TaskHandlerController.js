app.controller('TaskHandler', ['$scope', '$window', '$http', 'isRealTimeDB', 'timeout', 'sharedService',
    function($scope, $window, $http, isRealTimeDB, timeout, sharedService) {

        // Used to clear the interval in case of server error
        let intervalId;

        // Get server data each 3 seconds if live update
        $scope.$on('live-update', function() {
            intervalId = setInterval(() => $scope.getServerData(), 3000);
        });


        $scope.getServerData = function() {
            $http({
                method: 'GET',
                url: '/getServerData'
            }).then(
                function(res) {
                    // Write which instance is being solved
                    // TODO: check why the first call gives empty answer
                    $scope.solvingInstance = res.data[0];
                    $scope.solvingInstanceRemaining = res.data[1];
                },
                function(err) {
                    clearInterval(intervalId);
                    $window.alert(`An error occured: ${err}`);
                });
        };

        // Change timeout to to end the running evaluation
        // @param {String} type: Either 'increase' or 'decrease'.
        $scope.changeTimeout = function(type) {
            $http({
                method: 'POST',
                url: '/changeTimeout',
                data: {
                    'timeout': $('#smts-server-timeout').val(),
                    'type': type
                },
            }).then(
                function() {
                    // Prevent page redirect
                    event.preventDefault();
                },
                function(err) {
                    $window.alert(`An error occured: ${err}`);
                });
        };

        // Stop current running evaluation
        $scope.stop = function() {
            $http({
                method: 'POST',
                url: '/stop'
            }).then(
                function() {
                    // Do nothing
                },
                function(err) {
                    $window.alert(`An error occured: ${err}`);
                });
        };

    }]);