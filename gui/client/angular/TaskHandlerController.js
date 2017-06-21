
app.controller('TaskHandler',['$scope','$window','$http','realTimeDB', 'timeOut',function($scope,$window,$http, realTimeDB,timeOut){
    $scope.load = function() {
        // If real-time analysis execute every 3 seconds task Handler functions
        if(realTimeDB.value) {
            $('#solInst').removeClass('hidden');
            $('#task').removeClass('hidden');

            var interval = setInterval(function () {
                console.log("Contacting server....");
                $scope.getServerData();
            }, 3000);
        }


    };

    $scope.getServerData = function() {
        $http({
            method : 'GET',
            url : '/getServerData'
        }).then(function successCallback(response) {
            console.log(response.data);

            // Write which instance is being solved
            //TODO: check why the first call gives empty answer
            $scope.solvingInstance = response.data[0];
            $scope.solvingInstanceRemaining = response.data[1];

        }, function errorCallback(response) {
            // called asynchronously if an error occurs
            // or server returns response with an error status.
            $window.alert('An error occured!');
        });
    };

    // TODO: to prevent page redirection after posting move posting here and use "event.preventDefault();"
    $scope.increaseTimeout = function () {
        $http({
            method : 'POST',
            url : '/changeTimeout',
            data: {
                'timeout': $("input[name='timeout']").val(),
                'type' : "increase"
            },
        }).then(function successCallback(response) {
            event.preventDefault();

        }, function errorCallback(response) {
            // called asynchronously if an error occurs
            // or server returns response with an error status.
            $window.alert('An error occured!');
        });
    };

    $scope.decreaseTimeout = function () {
        $http({
            method : 'POST',
            url : '/changeTimeout',
            data: {
                'timeout': $("input[name='timeout']").val(),
                'type' : "decrease"
            },
        }).then(function successCallback(response) {

        }, function errorCallback(response) {
            // called asynchronously if an error occurs
            // or server returns response with an error status.
            $window.alert('An error occured!');
        });
    };

    $scope.stopSS = function () {
        $http({
            method : 'POST',
            url : '/stop'
        }).then(function successCallback(response) {

        }, function errorCallback(response) {
            // called asynchronously if an error occurs
            // or server returns response with an error status.
            $window.alert('An error occured!');
        });
    };

    // Attempt for submitting a db from angular
    // $( "#uploadDB" ).submit(function( event ) {
    //     event.preventDefault();
    //     var file_data = $("#db").prop("files")[0];
    //     console.log(file_data);
    //     // var formData = new FormData();
    //     // formData.append("file", file_data)
    //     // console.log(formData)
    //     var db = {"file": file_data};
    //     console.log(db)
    //
    //     $http({
    //         method : 'POST',
    //         url : '/upload',
    //         encType : "multipart/form-data",
    //         data: db
    //     }).then(function successCallback(response) {
    //         //put each entry of the response array in the table
    //
    //
    //     }, function errorCallback(response) {
    //         // called asynchronously if an error occurs
    //         // or server returns response with an error status.
    //         $window.alert('An error occured!');
    //     });
    // });

}]);