app.controller('TaskHandler', ['$scope', '$rootScope', '$window', '$http', 'sharedService', 'sharedTree', 'currentRow',
    function($scope, $rootScope, $window, $http, sharedService, sharedTree, currentRow) {

        // Update instance data values in server container
        $scope.$on('update-instance-data', function(e, instanceData) {
            $scope.instanceName = instanceData.name || 'Nothing';
            $scope.instanceTime = instanceData.time;
            $scope.instanceLeft = instanceData.left;
        });

        // Change timeout to to end the running evaluation
        // @param {String} type: Either 'increase' or 'decrease'.
        $scope.changeTimeout = function(type) {
            let delta = document.getElementById('smts-server-timeout').value || 0;
            if (type === 'decrease') delta *= -1;
            $http({method: 'POST', url: '/changeTimeout', data: {'delta': delta},}).then(
                function() {
                    // Do nothing
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