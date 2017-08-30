app.controller('EventController', ['$scope', '$rootScope', '$window', '$http', 'sharedService',
    function($scope, $rootScope, $window, $http, sharedService) {

        // Update events on instance selected
        $scope.$on('select-instance', function() {
            // Check if events table is scrolled to bottom. The check has to be
            // performed here, because later the scroll will change.
            let isScrollBottom = smts.events.isScrollBottom();

            // Make timeline
            let events = smts.tree.tree.getEvents(smts.events.index + 1);
            $scope.events = events;

            // Select last event
            $scope.selectEvent($scope.events[$scope.events.length - 1], $scope.events.length - 1, isScrollBottom);
        });

        // Show tree up to clicked event
        // @param {TreeManager.Event} event: The event to be selected.
        // @param {Number} index: Index of the last row in the database needed
        // to build the tree. E.g.: index = 16 means 'read the first 16 rows in
        // the database'.
        // @param {Boolean} isScrollBottom: Tells whether the events table is
        // already scrolled to the bottom. If yes, then the scroll is updated
        // to keep it to the bottom, in order to always have the most recent
        // event in sight, in case of live update.
        $scope.selectEvent = function(event, index, isScrollBottom) {
            // Build tree
            smts.events.index = index;
            smts.tree.tree.arrangeTree(smts.events.index);
            smts.tree.build();

            // Update data table
            let nodeName = event.getMainNode();
            smts.data.update(smts.tree.tree.root.getNode(nodeName), 'node');

            // Update timeline
            smts.timeline.update(event);

            // Update events and instances tables
            // N.B.: Angular doesn't offer a `ready` functionality, so it
            // is impossible to detect when the DOM elements are actually
            // ready, thus the functions are called asyncronously.
            window.setTimeout(function() {
                smts.events.highlight([event]);
                smts.events.update(smts.tree.tree.selectedNodes);
                smts.solvers.update(smts.tree.tree.selectedNodes);
                if (isScrollBottom) {
                    smts.events.scrollBottom();
                }
            }, 0);

            // Focus the events table for fast arrow selection
            document.querySelector('#smts-events-table > tbody').focus();

            // Notify selected element
            sharedService.broadcastSelectEvent();
        };

        // Shift selected element above or below current one
        // @param {event} e: The event triggered by a keyboard input.
        $scope.shiftSelected = function(e) {
            if (e.key === 'ArrowUp') {
                e.preventDefault();
                smts.events.shift('up');
            } else if (e.key === 'ArrowDown') {
                e.preventDefault();
                smts.events.shift('down');
            }
        };

        // Show all events in events table
        $scope.showAll = function() {
            smts.events.showAll();
        };

        // Show only events in events table related to selected nodes
        $scope.showSelected = function() {
            smts.events.showSelected(smts.tree.tree.selectedNodes);
        };
    }]);