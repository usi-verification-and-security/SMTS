app.controller('InstancesController', ['$scope', '$rootScope', 'currentRow', 'sharedTree', 'isRealTimeDB', 'timeout', 'DBcontent', '$window', '$http', 'sharedService',
    function($scope, $rootScope, currentRow, sharedTree, isRealTimeDB, timeout, DBcontent, $window, $http, sharedService) {

        // Load all instances in instances table
        $scope.load = function() {
            // Check if real time analysis
            this.isRealTime();

            // Get all instances
            $http({
                method: 'GET',
                url: '/getInstances'
            }).then(
                function(res) {
                    $scope.instances = res.data;
                },
                function(err) {
                    $window.alert(`An error occured: ${err}`);
                });
        };

        // Check if it is real time analysis or not
        // It hides the database container if it is real time analysis, or the
        // server container otherwise. It also broadcasts the signal that real
        // time analysis is on.
        $scope.isRealTime = function() {
            $http({
                method: 'GET',
                url: '/getRealTime'
            }).then(
                function(res) {
                    if (res.data === true) {
                        isRealTimeDB.value = true;
                        // Hide database container which is not relevant in this case
                        $('#smts-database-container').addClass('smts-hidden');
                        // Update timeout value
                        $('#smts-server-timeout').val = timeout.value;
                        // Notify it is live update
                        sharedService.broadcastLiveUpdate();
                    }
                    else {
                        isRealTimeDB.value = false;
                        // Hide server container which is not relevant in this case
                        $('#smts-server-container').addClass('smts-hidden');
                    }

                },
                function(err) {
                    $window.alert(`An error occured: ${err}`);
                });
        };


        // Select an instance and load its data from database
        $scope.selectInstance = function(instance) {
            smts.tables.instances.highlight(instance);

            // Get tree data
            this.loadData(instance);

            // If real-time analysis ask every 10 seconds for db content
            if (isRealTimeDB.value) {
                setInterval(() => this.loadData(instance), 10000);
            }
        };

        // Load database data relative to a particular instance
        // It gets the database data and generates a tree with it.
        // @param {Instance} instance: Instance to load data from.
        $scope.loadData = function(instance) {
            $http({
                method: 'GET',
                url: '/get/' + instance.name
            }).then(
                function successCallback(res) {
                    if (DBcontent.value !== res.data) {
                        // TODO: solve bug with DBcontent.value
                        // console.log(DBcontent.value) // BUG
                        DBcontent.value = res.data;

                        // Initialize tree
                        sharedTree.tree = new TreeManager.Tree();
                        sharedTree.tree.createEvents(res.data);
                        sharedTree.tree.initializeSolvers(res.data);
                        currentRow.value = res.data.length - 1;

                        // Notify an instance has been selected
                        sharedService.broadcastSelectInstance();
                    }
                },
                function errorCallback(err) {
                    // Called asynchronously if an error occurs
                    // or server returns response with an error status.
                    $window.alert(`An error occured: ${err}`);
                });
        };
    }]);
