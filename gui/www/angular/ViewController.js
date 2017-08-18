app.controller('ViewController', ['$scope', '$rootScope', '$window', '$http', 'sharedService', 'sharedTree', 'currentRow',
    function($scope, $rootScope, $window, $http, sharedService, sharedTree, currentRow) {

        // Page initial setup
        $(window).load(function() {
            let switchOptions = {
                size:     'mini',
                onText:   'or',
                offText:  'and',
                onColor:  'primary',
                offColor: 'primary'
            };

            let variablesSwitch = $('#smts-content-cnf-literal-info-variables-switch');
            variablesSwitch.bootstrapSwitch(switchOptions);
            variablesSwitch.on('switchChange.bootstrapSwitch', smts.cnf.updateSelectedNodes);

            let clausesSwitch = $('#smts-content-cnf-literal-info-clauses-switch');
            clausesSwitch.bootstrapSwitch(switchOptions);
            clausesSwitch.on('switchChange.bootstrapSwitch', smts.cnf.updateSelectedNodes);
        });

        // Regenerate tree on window resize
        $(window).resize(function() {
            if (sharedTree.tree) {
                sharedTree.tree.arrangeTree(currentRow.value);
                smts.tree.make(sharedTree.tree, smts.tree.getPosition());
            }
        });
    }]);