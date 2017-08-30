let app = angular.module('myApp', ['ngFileUpload'])
    // `sharedService` contains functions to broadcast signals that will be
    // received by other AngularJS components
    .factory('sharedService', function ($rootScope) {
        let sharedService = {};

        // Send signal when instance is clicked
        sharedService.broadcastSelectInstance = function () {
            $rootScope.$broadcast('select-instance');
        };

        // Send signal when event is clicked
        sharedService.broadcastSelectEvent = function () {
            $rootScope.$broadcast('select-event');
        };

        // Send signal with updated solving instance data
        sharedService.broadcastUpdateInstanceData = function (instanceData) {
            $rootScope.$broadcast('update-instance-data', instanceData);
        };

        return sharedService;
    });
