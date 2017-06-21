
app.controller('InstancesController',['$scope','$rootScope','currentRow','sharedTree','realTimeDB', 'timeOut','DBcontent','$window','$http','sharedService',function($scope,$rootScope, currentRow,sharedTree,realTimeDB,timeOut,DBcontent,$window,$http,sharedService){

    $scope.load = function() {
        $http({
            method : 'GET',
            url : '/getInstances'
        }).then(function successCallback(response) {
            //put each entry of the response array in the table
            // console.log(response.data)
            $scope.entries = response.data;
            if(realTimeDB.value == false){
                $('#solInst').addClass('hidden');
                $('#newInst').addClass('hidden');
                $('#task').addClass('hidden');
            }
            else{
                $("input[name='timeout']").val(timeOut.value);
            }

        }, function errorCallback(response) {
            // called asynchronously if an error occurs
            // or server returns response with an error status.
            $window.alert('An error occured!');
        });
    };

    $scope.clickEvent = function($event,x){
        // Highlight selected instance
        $('.instance-container table tr').removeClass("highlight");
        var query = '.instance-container table tr[data-instance="' + x.name +'"]';
        $(query).addClass("highlight");

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
            url : '/get/' + x.name
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
