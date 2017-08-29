app.controller('InstancesController', ['$scope', '$rootScope', '$window', '$http', 'sharedService', 'sharedTree', 'currentRow',
    function($scope, $rootScope, $window, $http, sharedService, sharedTree, currentRow) {


        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // CONSTANTS AND GLOBAL VARIABLES
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Time intervals for update functions, expressend in milliseconds
        const INTERVAL_UPDATE_SOLVING_INFO = 1000;
        const INTERVAL_UPDATE_INSTANCES    = 5000;
        const INTERVAL_UPDATE_EVENTS       = 5000;


        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // MAIN
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Start application
        $scope.load = function() {
            $http({method: 'GET', url: '/info'}).then(
                function(res) {
                    let info = res.data;
                    $scope.isLive = info.isLive;
                    document.getElementById('smts-title-version').innerHTML = `v${info.version}`;
                    if (info.isLive) {
                        // Show server (live) mode elements
                        $scope.hide('live');

                        // Request updates if real time analysis
                        $scope.updateSolvingInfoIntervalId =
                            window.setInterval($scope.updateSolvingInfo, INTERVAL_UPDATE_SOLVING_INFO);
                        $scope.updateInstancesIntervalId =
                            window.setInterval($scope.updateInstances, INTERVAL_UPDATE_INSTANCES);
                    }
                    else {
                        // Show database mode elements
                        $scope.hide('database');
                    }

                    // Show instances table and utilities
                    document.getElementById('smts-instances-container').classList.remove('smts-invisible');
                    document.getElementById('smts-utilities-container').classList.remove('smts-invisible');

                    // Get instances
                    $scope.getInstances();
                }, $scope.error);
        };

        // Get all instances and load them in instances table
        $scope.getInstances = function() {
            $http({method: 'GET', url: '/instances'}).then(
                function(res) {
                    $scope.instances = res.data;
                }, $scope.error);
        };

        // Hide all elements associated to a particular mode
        // HTML elements with class name `smts-hidden-on-mode-${mode}` are made
        // hidden. This function is used to change the visibility of elements
        // once it is known if it's live mode or not.
        // @param {string} mode: Either `live` or `database`.
        $scope.hide = function(mode) {
            let html = $('html').get(0);
            let style = document.createElement('style');
            style.type = 'text/css';
            style.innerHTML = `.smts-hide-on-mode-${mode} { display: none !important; }`;
            html.appendChild(style);

            // To use once the `revert` keyword will be available in CSS
            // html.style.setProperty(`--visibility-mode-${mode}`, 'revert');
        };


        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // INSTANCE
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Select an instance and load its data from database
        // @param {Instance} instance: Selected instance.
        $scope.selectInstance = function(instance) {
            // Highlight instance
            smts.tables.instances.highlight([instance]);

            // First time an instance is selected
            document.getElementById('smts-content').classList.remove('smts-hidden');

            // Get tree data
            this.getEvents(instance);

            if ($scope.isLive) {
                // Ask repeatedly for db content
                clearInterval($scope.updateEventsIntervalId); // Clear previous instance updates
                $scope.updateEventsIntervalId =
                    setInterval($scope.updateEvents.bind(null, instance), INTERVAL_UPDATE_EVENTS);
            }
        };

        // Load database data relative to a particular instance
        // It gets the database data and generates a tree with it.
        // @param {Instance} instance: Instance to load data from.
        // @param (Number) [optional] id: If present, requests events only from
        // given id, and appends them to already existing events. Otherwise,
        // events are completly overwritten.
        $scope.getEvents = function(instance, eventId) {
            let query = eventId ? `?id=${eventId}` : ``;
            $http({method: 'GET', url: `/events/${instance.name}${query}`}).then(
                function(res) {
                    let events = res.data || [];

                    if (!eventId) $scope.events = []; // Reset events if new request

                    // Return if we receive an empty update from the current instance
                    let instanceName = smts.tables.instances.getSelected();
                    if (instance.name === instanceName && !events.length) {
                        return;
                    }
                    $scope.events = $scope.events.concat(events);

                    // Update last event
                    let lastEvent = $scope.events[$scope.events.length - 1];
                    if (lastEvent) $scope.lastEventId = lastEvent.id;

                    // Initialize tree
                    sharedTree.tree = new TreeManager.Tree();
                    sharedTree.tree.createEvents($scope.events);
                    sharedTree.tree.initializeSolvers($scope.events);
                    currentRow.value = $scope.events.length - 1;

                    // Initialize timeline
                    smts.timeline.clear();
                    smts.timeline.make(sharedTree.tree.getEvents(currentRow.value + 1));

                    // Notify an instance has been selected
                    sharedService.broadcastSelectInstance();
                }, $scope.error);
        };


        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // UPDATES
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Get current solving instance info to show in server container
        $scope.updateSolvingInfo = function() {
            $http({method: 'GET', url: '/solvingInfo'}).then(
                function(res) {
                    let instanceData = res.data;
                    // Bold running instance
                    smts.tables.instances.bold([instanceData]);
                    // Broadcast running instance data
                    sharedService.broadcastUpdateInstanceData(instanceData);
                }, $scope.error);
        };

        // Get instances and add new ones to instances table
        $scope.updateInstances = function() {
            $http({method: 'GET', url: '/instances'}).then(
                function(res) {
                    let newInstances = res.data;
                    for (let newInstance of newInstances) {
                        // Check if newInstance doesn't match any of those already present in table
                        if ($scope.instances.every(instance => instance.name !== newInstance.name)) {
                            $scope.instances.push(newInstance);
                        }
                    }
                }, $scope.error);
        };

        // Get events starting from the first id not present in events table
        // @param {Instance} instance: Instance to which the events are
        // associated.
        $scope.updateEvents = function(instance) {
            $scope.getEvents(instance, $scope.lastEventId + 1);
        };


        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // OTHER
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Handle errors for HTTP requests
        // @param {Error} err: The error received.
        $scope.error = function(err) {
            $scope.clearAllIntervals();
            smts.tools.error(err);
        };

        // Clear all intervals
        // In case of error, all real time requests to database are suppressed
        $scope.clearAllIntervals = function() {
            if ($scope.updateSolvingInfoIntervalId) {
                clearInterval($scope.updateSolvingInfoIntervalId);
                $scope.updateSolvingInfoIntervalId = null;
            }
            if ($scope.updateInstancesIntervalId) {
                clearInterval($scope.updateInstancesIntervalId);
                $scope.updateInstancesIntervalId = null;
            }
            if ($scope.updateEventsIntervalId) {
                clearInterval($scope.updateEventsIntervalId);
                $scope.updateEventsIntervalId = null;
            }
        }
    }]);
