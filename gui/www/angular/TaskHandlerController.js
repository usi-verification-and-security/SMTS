app.controller('TaskHandler', ['$scope', '$window', '$http', 'sharedService',
    function($scope, $window, $http, sharedService) {

        $scope.$on('update-instance-data', function(e, instanceData) {
            $scope.instanceName = instanceData.name || 'Nothing';
            $scope.instanceTime = instanceData.time;
            $scope.instanceLeft = instanceData.left;
        });

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