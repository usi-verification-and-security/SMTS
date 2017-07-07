app.controller('ViewController', ['$scope', '$rootScope', 'currentRow', 'sharedTree', '$window', '$http', 'sharedService',
    function ($scope, $rootScope, currentRow, sharedTree, $window, $http, sharedService) {
        $scope.$on('handleBroadcast', function () { // This is called when an instance is selected
            sharedTree.tree.arrangeTree(currentRow.value);
            let treeView = sharedTree.tree.getTreeView();
            generateDomTree(treeView, sharedTree.tree.getSelectedNodeNames(currentRow.value));
        });
    }]);