var app = angular.module('myApp', ['ngFileUpload'])

// Variable used to keep track how many rows of the db needs to be read
    .value('currentRow', {value: 0})
    // Variable used to say that we are in real-time analysis
    .value('realTimeDB', {value: undefined})

    .value('DBcontent', {value: null})
    // Variable used to keep track of the current solving timeout in case of real-time analysis (1000 by default)
    .value('timeOut', {value: 1000})

    .factory('sharedService', function ($rootScope) {
        var sharedService = {};

        // This is used to show solvers, tree and events when an instance is selected
        sharedService.broadcastItem = function () {
            $rootScope.$broadcast('handleBroadcast');
        };

        // This is used to show in solver view solver's new status when an event is clicked and a new tree is displayed
        sharedService.broadcastItem2 = function () {
            $rootScope.$broadcast('handleBroadcast2');
        };

        // This is used to show in solver view solver's new status when an event is clicked and a new tree is displayed
        sharedService.broadcastItem3 = function () {
            $rootScope.$broadcast('handleBroadcast3');
        };

        return sharedService;
    })

    //Tree
    .factory('sharedTree', function () {
        var tree = new TreeManager.Tree();
        return tree;

    });
