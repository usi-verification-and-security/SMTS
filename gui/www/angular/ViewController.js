app.controller('ViewController', ['$scope', '$rootScope', 'currentRow', 'sharedTree', '$window', '$http', 'sharedService',
    function($scope, $rootScope, currentRow, sharedTree, $window, $http, sharedService) {

        // Regenerate tree on window resize
        $(window).resize(function() {
            if (sharedTree.tree) {
                sharedTree.tree.arrangeTree(currentRow.value);
                generateDomTree(sharedTree.tree, smts.tree.getPosition());
            }
        });
    }]);