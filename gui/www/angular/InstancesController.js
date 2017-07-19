app.controller('InstancesController', ['$scope', '$rootScope', 'currentRow', 'sharedTree', '$window', '$http', 'sharedService',
    function($scope, $rootScope, currentRow, sharedTree, $window, $http, sharedService) {

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // MAIN
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Start application
        $scope.load = function() {
            $http({method: 'GET', url: '/info'}).then(
                function(res) {
                    let info = res.data;
                    $scope.isRealTime = info.isRealTime;
                    // TODO: set version in title with info.version

                    if (info.isRealTime) {
                        // Show server container
                        $('#smts-server-container').removeClass('smts-hidden');

                        // Request updates if real time analysis
                        $scope.updateSolvingInfoIntervalId =
                            window.setInterval($scope.updateSolvingInfo, 3000);
                        $scope.updateInstancesIntervalId =
                            window.setInterval($scope.updateInstances, 5000);
                    }
                    else {
                        // Show database container
                        $('#smts-database-container').removeClass('smts-hidden');
                    }

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

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // INSTANCE
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Select an instance and load its data from database
        // @param {Instance} instance: Selected instance.
        $scope.selectInstance = function(instance) {
            // Highlight instance
            smts.tables.instances.highlight([instance]);

            // Unhide other containers
            // Useful the first time an instance is selected
            document.getElementById('smts-column-2').classList.remove('smts-hidden');
            document.getElementById('smts-column-3').classList.remove('smts-hidden');
            document.getElementById('smts-timeline').classList.remove('smts-hidden');

            // Get tree data
            this.getEvents(instance);

            // If real-time analysis, ask every 5 seconds for db content
            if ($scope.isRealTime) {
                $scope.updateEventsIntervalId =
                    setInterval($scope.updateEvents.bind(null, instance), 5000);
            }
        };

        // Load database data relative to a particular instance
        // It gets the database data and generates a tree with it.
        // @param {Instance} instance: Instance to load data from.
        // @param (Number) id (optional): If present, requests events only from
        // given id, and appends them to already existing events. Otherwise,
        // events are completly overwritten.
        $scope.getEvents = function(instance, id) {
            let query = id ? `?id=${id}` : ``;
            $http({method: 'GET', url: `/events/${instance.name}${query}`}).then(
                function(res) {
                    if (!id) $scope.events = []; // Reset events if new request
                    $scope.events = $scope.events.concat(res.data);

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

        $scope.updateSolvingInfo = function() {
            $http({method: 'GET', url: '/getSolvingInfo'}).then(
                function(res) {
                    $scope.instanceName = res.data.name;
                    $scope.instanceTime = res.data.time;
                    $scope.instanceLeft = res.data.left;

                    // Stop requesting events update if the instance is solved
                    if (!res.data.name && $scope.updateEventsIntervalId) {
                        clearInterval($scope.updateEventsIntervalId);
                        $scope.updateEventsIntervalId = null;
                    }
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

        $scope.updateEvents = function(instance) {
            $scope.getEvents(instance, instance.id);
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
