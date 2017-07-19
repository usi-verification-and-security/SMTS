app.controller('TaskHandler', ['$scope', '$window', '$http', 'sharedService',
    function($scope, $window, $http, sharedService) {

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
                    event.preventDefault(); // Prevent page redirect
                }, smts.tools.error);
        };

        // Stop current running evaluation
        $scope.stop = function() {
            $http({method: 'POST', url: '/stop'}).then(
                function() {
                    // Do nothing
                }, smts.tools.error);
        };

    }]);