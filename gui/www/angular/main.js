let app = angular.module('myApp', ['ngFileUpload'])

    // N.B.: All values are wrapped around an object in order not to lose the reference

    // {Number}: How many rows of the db needs to be read
    .value('currentRow', {value: 0})

    // {Object}: Contain functions to broadcast signals received by other Angular components
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
    })

    // {TreeManager.Tree}: Structure representing the tree made from the database
    .factory('sharedTree', function () {
        return new TreeManager.Tree();
    });
