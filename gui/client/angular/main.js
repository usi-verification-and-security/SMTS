let app = angular.module('myApp', ['ngFileUpload'])

    // {Number}: How many rows of the db needs to be read
    .value('currentRow', {value: 0})

    // {Boolean}: true if real-time analysis, false otherwise
    .value('realTimeDB', {value: false})

    // TODO: describe
    .value('DBcontent', {value: null})

    // {Number}: Current solving timeout in case of real-time analysis (1000 seconds by default)
    .value('timeOut', {value: 1000})

    // {Function}: Send broadcast signals received by othe Angular components
    .factory('sharedService', function ($rootScope) {

        let sharedService = {};

        // This is used to show solvers, tree and events when an instance is selected
        sharedService.broadcastSelectInstance = function () {
            $rootScope.$broadcast('select-instance');
        };

        // This is used to show in solver view solver's new status when an event is clicked and a new tree is displayed
        sharedService.broadcastSelectEvent = function () {
            $rootScope.$broadcast('select-event');
        };

        // This is used to show in solver view solver's new status when an event is clicked and a new tree is displayed
        sharedService.broadcastLiveUpdate = function () {
            $rootScope.$broadcast('live-update');
        };

        return sharedService;
    })

    // {TreeManager.Tree}: Structure representing the tree made from the database
    .factory('sharedTree', function () {
        return new TreeManager.Tree();
    });
