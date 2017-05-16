
angular.module('myApp', ['ngFileUpload'])

    // Variable used to keep track how many rows of the db needs to be read
    .value('currentRow', { value: 0})

    .value('eventRow', { value: undefined})

    .value('instanceRow', { value: undefined})

    .value('realTimeDB', { value: false})

    .value('DBcontent', { value: null})

    .factory('sharedService', function($rootScope) {
    var sharedService = {};

    // This is used to show solvers, tree and events when an instance is selected
    sharedService.broadcastItem = function() {
        $rootScope.$broadcast('handleBroadcast');
    };

    // This is used to show in solver view solver's new status when an event is clicked and a new tree is displayed
    sharedService.broadcastItem2 = function() {
        $rootScope.$broadcast('handleBroadcast2');
    };

    return sharedService;
    })

    //Tree
    .factory('sharedTree', function() {
        var tree = new TreeManager.Tree();
        return tree;

    })

    .controller('TreeController',['$scope','$rootScope','currentRow','$window','$http','sharedService',function($scope,$rootScope, currentRow,$window,$http,sharedService){

    }])

    .controller('EventController',['$scope','$rootScope','currentRow','eventRow','sharedTree','$window','$http', 'sharedService',function($scope,$rootScope, currentRow,eventRow,sharedTree,$window,$http,sharedService){

        $scope.$on('handleBroadcast', function() { // This is called when an instance is selected
            var eventEntries = sharedTree.tree.getEvents(currentRow.value);
            // console.log(eventEntries)
            $scope.entries = eventEntries;
            $scope.initTimeline(eventEntries); // Initialize timeline

        });

        $scope.initTimeline = function(events){
            clearTimeline();
            allEvents = events;
            makeCircles();

            $(".circle").mouseenter(function() {
                $(this).addClass("hover");
            });

            $(".circle").mouseleave(function() {
                $(this).removeClass("hover");
            });

            $(".circle").click(function() {
                var spanNum = $(this).attr("id");
                // console.log(spanNum);
                selectEvent(spanNum)
                //Simulate event click to rebuild the tree
                var find = "event" + spanNum;
                var row = document.getElementById(find);
                row.click();

                // TODO: Scroll table till selected event
                // NOT spanNum but table row!
                // var ypos = $(".d5_2 tr:eq(' + spanNum +')").offset();
                // $('.d5' ).scrollTop( ypos.top );
            });
        };

        // Show tree up to clicked event
        $scope.showEvent = function($event,$index,x){
            if(eventRow.value != undefined){
                eventRow.value.style.color= "black";
            }
            eventRow.value = $event.currentTarget;
            $event.currentTarget.style.color= "#0073e6";

            currentRow.value = $index;
            sharedTree.tree.arrangeTree(currentRow.value);
            var treeView = sharedTree.tree.getTreeView();
            getTreeJson(treeView);

            // Show event's data in dataView
            var object = JSON.parse(x.data);
            if(object == null){
                object = {};
            }
            var ppTable = prettyPrint(object);
            var tableName = "Event " + x.event;
            document.getElementById('d6_1').innerHTML = tableName.bold();
            var item = document.getElementById('d6_2');

            if(item.childNodes[0]){
                item.replaceChild(ppTable, item.childNodes[0]); //Replace existing table
            }
            else{
                item.appendChild(ppTable);
            }

            //Update timeline
            var circle = document.getElementById(x.id);
            if(circle != null){
                selectEvent(x.id);
            }
            else if((circle == null) && (timelineEvents.length > 1)){ // if timelineEvents.length < 2 then there isn' the timeline
                var index = findElwithSameTS(x.ts);
                selectEvent(index);
            }

            sharedService.broadcastItem2();
        }

    }])

    .controller('SolverController',['$scope','$rootScope','currentRow','sharedTree','$window','$http', 'sharedService',function($scope,$rootScope, currentRow,sharedTree,$window,$http,sharedService){
        $scope.$on('handleBroadcast', function() { // This is called when an instance is selected
            $scope.showSolver();
        });
        $scope.$on('handleBroadcast2', function() { // This is called when an event is selected and the new tree shows up
            $scope.showSolver();
        });

        $scope.showSolver = function() {
            sharedTree.tree.assignSolvers(0,currentRow.value);
            $scope.entries = sharedTree.tree.solvers;

        };

        $scope.clickEvent = function($event,x){
            if(x.node){
                x.node = x.node.toString(); // transform to string or it will show and array
            }
            var ppTable = prettyPrint(x);
            document.getElementById('d6_1').innerHTML = "Solver".bold();
            var item = document.getElementById('d6_2');

            if(item.childNodes[0]){
                item.replaceChild(ppTable, item.childNodes[0]); //Replace existing table
            }
            else{
                item.appendChild(ppTable);
            }

        }

    }])

    .controller('InstancesController',['$scope','$rootScope','currentRow','instanceRow','sharedTree','realTimeDB','DBcontent','$window','$http','sharedService',function($scope,$rootScope, currentRow,instanceRow,sharedTree,realTimeDB,DBcontent,$window,$http,sharedService){

        $scope.load = function() {
            $http({
                method : 'GET',
                url : 'http://localhost:3000/getInstances'
            }).then(function successCallback(response) {
                //put each entry of the response array in the table
                $scope.entries = response.data;

            }, function errorCallback(response) {
                // called asynchronously if an error occurs
                // or server returns response with an error status.
                $window.alert('An error occured!');
            });
        };

        $scope.clickEvent = function($event,x){
            if(instanceRow.value != undefined){
                instanceRow.value.style.color= "black";
            }
            instanceRow.value = $event.currentTarget;
            $event.currentTarget.style.color= "#0073e6";

            // If real-time analysis ask every 10 seconds for db content otherwise just once
            if(realTimeDB.value) {
                this.getTree(x);
                var interval = setInterval(function () {
                    console.log("DB content asked.");
                    this.getTree(x);
                }, 10000);
            }
            else{
                console.log("DB content asked for passed execution.");
                this.getTree(x); // show corresponding tree
            }

        };

        $scope.getTree = function(x) {
            $http({
                method : 'GET',
                url : 'http://localhost:3000/get/' + x.name
            }).then(function successCallback(response) {
                if(DBcontent.value != response.data){
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
                $window.alert('An error occured!');
            });


        };

    }])

    .controller('ViewController',['$scope','$rootScope','currentRow','sharedTree','$window','$http', 'sharedService',function($scope,$rootScope, currentRow,sharedTree,$window,$http,sharedService){
        $scope.$on('handleBroadcast', function() { // This is called when an instance is selected
            sharedTree.tree.arrangeTree(currentRow.value);
            var treeView = sharedTree.tree.getTreeView();
            getTreeJson(treeView);
        });

    }]);




