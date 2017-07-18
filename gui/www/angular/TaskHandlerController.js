app.controller('TaskHandler', ['$scope', '$window', '$http', 'timeout', 'sharedService',
    function($scope, $window, $http, timeout, sharedService) {

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