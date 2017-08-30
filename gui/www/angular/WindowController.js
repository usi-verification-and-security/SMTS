app.controller('WindowController', ['$scope', '$rootScope', '$window', '$http', 'sharedService',
    function($scope, $rootScope, $window, $http, sharedService) {

        // Page initial setup
        $(window).load(function() {
            // Convert checkboxes to switches
            // See: http://bootstrapswitch.com/
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
            if (smts.tree.tree) {
                smts.tree.tree.arrangeTree(smts.events.index);
                smts.tree.build();
            }
        });
    }]);