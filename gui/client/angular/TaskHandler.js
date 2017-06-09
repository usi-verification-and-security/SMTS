
app.controller('TaskHandler',['$scope','$window','$http','realTimeDB',function($scope,$window,$http, realTimeDB){
    $scope.load = function() {
        // If real-time analysis execute every 5 seconds task Handler functions
        if(realTimeDB.value) {
            $('#solInst').removeClass('hidden');

            var interval = setInterval(function () {
                console.log("Contacting server....");
                $scope.getServerData();
            }, 5000);
        }


    };

    $scope.getServerData = function() {
        $http({
            method : 'GET',
            url : '/getServerData'
        }).then(function successCallback(response) {
            console.log(response.data)

            // Write which instance is being solved
            //TODO: check why the first call gives empty answer
            $scope.solvingInstance = response.data[0];

        }, function errorCallback(response) {
            // called asynchronously if an error occurs
            // or server returns response with an error status.
            $window.alert('An error occured!');
        });
    };

    // TODO: to prevent page redirection after posting move posting here and use "event.preventDefault();"
    $scope.setTimeout = function () {
        $http({
            method : 'POST',
            url : '/change'
        }).then(function successCallback(response) {
            //put each entry of the response array in the table


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
            //put each entry of the response array in the table


        }, function errorCallback(response) {
            // called asynchronously if an error occurs
            // or server returns response with an error status.
            $window.alert('An error occured!');
        });
    };

    $scope.uploadDb = function (file) {
        console.log(file)
        $http({
            method : 'POST',
            url : '/upload',
            encType : "multipart/form-data",
            params : {'files': file}
        }).then(function successCallback(response) {
            //put each entry of the response array in the table


        }, function errorCallback(response) {
            // called asynchronously if an error occurs
            // or server returns response with an error status.
            $window.alert('An error occured!');
        });
    };

}]);