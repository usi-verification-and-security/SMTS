app.controller('ViewController', ['$scope', '$rootScope', 'currentRow', 'sharedTree', '$window', '$http', 'sharedService',
    function($scope, $rootScope, currentRow, sharedTree, $window, $http, sharedService) {

        // Generate tree in tree view when an instance is selected
        $scope.$on('select-instance', function() {
            sharedTree.tree.arrangeTree(currentRow.value);
            generateDomTree(sharedTree.tree, null);
        });

        // Regenerate tree on window resize
        $(window).resize(function() {
            if (sharedTree.tree) {
                sharedTree.tree.arrangeTree(currentRow.value);
                generateDomTree(sharedTree.tree, smts.tree.getPosition());
            }
        });
    }]);