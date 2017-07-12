app.controller('InstancesController', ['$scope', '$rootScope', 'currentRow', 'sharedTree', 'realTimeDB', 'timeOut', 'DBcontent', '$window', '$http', 'sharedService',
    function ($scope, $rootScope, currentRow, sharedTree, realTimeDB, timeOut, DBcontent, $window, $http, sharedService) {

        $scope.load = function () {
            this.isRealTime();

            $http({
                method: 'GET',
                url: '/getInstances'
            }).then(function successCallback(response) {
                $scope.instances = response.data;

            }, function errorCallback(response) {
                $window.alert('An error occured: ' + response);
            });
        };

        // Check if is real time analysis or not
        $scope.isRealTime = function () {
            $http({
                method: 'GET',
                url: '/getRealTime'
            }).then(function successCallback(response) {

                if (response.data === true) {
                    realTimeDB.value = true;

                    $("input[name='timeout']").val(timeOut.value);

                    $('#solInst').removeClass('smts-hidden');
                    $('#task').removeClass('smts-hidden');
                    $('#newInst').removeClass('smts-hidden');

                    $('#newDB').addClass('smts-hidden');

                    sharedService.broadcastItem3(); // Show events, tree and solvers
                }
                else {
                    realTimeDB.value = false;
                    $('#newDB').removeClass('smts-hidden');
                }

            }, function errorCallback(response) {
                $window.alert('An error occured: ' + response);
            });
        };

        $scope.clickEvent = function ($event, instance) {
            // Highlight selected instance

            let row = $event.target.parentNode;
            $(row.parentNode).children('tr').removeClass('smts-highlight');
            row.classList.add('smts-highlight');

            // If real-time analysis ask every 10 seconds for db content otherwise just once
            if (realTimeDB.value) {
                this.getTree(instance);
                setInterval(function () {
                    //console.log("DB content asked.");
                    this.getTree(instance);
                }, 10000);
            }
            else {
                //console.log("DB content asked for passed execution.");
                this.getTree(instance); // show corresponding tree
            }
        };

        $scope.getTree = function (x) {
            $http({
                method: 'GET',
                url: '/get/' + x.name
            }).then(function successCallback(response) {
                if (DBcontent.value !== response.data) {
                    //TODO: solve bug with DBcontent.value
                    // console.log("inside if")
                    // console.log(DBcontent.value) // BUG
                    DBcontent.value = response.data;

                    // Initialize tree
                    sharedTree.tree = new TreeManager.Tree();
                    sharedTree.tree.createEvents(response.data);
                    currentRow.value = response.data.length - 1;
                    sharedTree.tree.initializeSolvers(response.data);

                    sharedService.broadcastItem(); // Show events, tree and solvers
                }

            }, function errorCallback(response) {
                // called asynchronously if an error occurs
                // or server returns response with an error status.
                // $window.alert('An error occured!');
                console.log(response);
            });
        };
    }]);
