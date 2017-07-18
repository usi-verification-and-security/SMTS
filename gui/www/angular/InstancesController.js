app.controller('InstancesController', ['$scope', '$rootScope', 'currentRow', 'sharedTree', 'timeout', 'DBcontent', '$window', '$http', 'sharedService',
    function($scope, $rootScope, currentRow, sharedTree, timeout, DBcontent, $window, $http, sharedService) {

        // Start application
        $scope.load = function() {
            $http({method: 'GET', url: '/info'}).then(
                function(res) {
                    let info = res.data;
                    $scope.isRealTime = info.isRealTime;
                    // TODO: set version in title with info.version

                    $scope.getInstances();

                    $scope.showTaskHandler(info.isRealTime);

                    if (info.isRealTime) {
                        $scope.updateSolvingInfoIntervalId =
                            window.setInterval($scope.updateSolvingInfo, 3000);
                        $scope.updateInstancesIntervalId =
                            window.setInterval($scope.updateInstances, 5000);
                    }
                },
                function(err) {
                    smts.tools.error(err);
                }
            );
        };

        $scope.updateSolvingInfo = function() {
            $http({method: 'GET', url: '/getSolvingInfo'}).then(
                function(res) {
                    // Write which instance is being solved
                    let name = res.data[0] !== 'Empty' ? `${res.data[0]}` : 'Nothing';
                    let timeLeft = res.data[1] !== 'Empty' ? `${res.data[1]}s` : '0s';
                    document.getElementById('smts-server-solving-instance').innerHTML = name;
                    document.getElementById('smts-server-solving-time-left').innerHTML = timeLeft;

                    if (name === 'Nothing') {
                        // Stop requesting events update
                        if ($scope.updateEventsIntervalId) {
                            clearInterval($scope.updateEventsIntervalId);
                            $scope.updateEventsIntervalId = null;
                        }
                    }
                },
                function(err) {
                    $scope.clearAllIntervals();
                    smts.tools.error(err);
                });
        };

        // Get all instances and load them in instances table
        $scope.getInstances = function() {
            $http({method: 'GET', url: '/instances'}).then(
                function(res) {
                    $scope.instances = res.data;
                },
                function(err) {
                    $scope.clearAllIntervals();
                    smts.tools.error(err);
                });
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
                },
                function(err) {
                    $scope.clearAllIntervals();
                    smts.tools.error(err);
                });
        };

        // Show the server/database container
        // It displays the server container if it is real time analysis, or the
        // database container otherwise.
        $scope.showTaskHandler = function() {
            if ($scope.isRealTime) {
                // Show server container
                $('#smts-server-container').removeClass('smts-hidden');
                // Update timeout value
                $('#smts-server-timeout').val = timeout.value;
            }
            else {
                // Show database container
                $('#smts-database-container').removeClass('smts-hidden');
            }
        };

        // Select an instance and load its data from database
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

        $scope.updateEvents = function(instance) {
            console.log('UPDATE:', instance.name);
        };

        // Load database data relative to a particular instance
        // It gets the database data and generates a tree with it.
        // @param {Instance} instance: Instance to load data from.
        $scope.getEvents = function(instance) {
            $http({method: 'GET', url: `/events/${instance.name}`}).then(
                function(res) {
                    if (DBcontent.value !== res.data) {
                        DBcontent.value = res.data;

                        // Initialize tree
                        sharedTree.tree = new TreeManager.Tree();
                        sharedTree.tree.createEvents(res.data);
                        sharedTree.tree.initializeSolvers(res.data);
                        currentRow.value = res.data.length - 1;

                        // Notify an instance has been selected
                        sharedService.broadcastSelectInstance();
                    }
                }, $scope.error);
        };

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
