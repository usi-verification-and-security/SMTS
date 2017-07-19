app.controller(
    'EventController', ['$scope', '$rootScope', 'currentRow', 'sharedTree', '$window', '$http', 'sharedService',
        function($scope, $rootScope, currentRow, sharedTree, $window, $http, sharedService) {

            // Update events on instance selected
            $scope.$on('select-instance', function() {
                // Make timeline
                let events = sharedTree.tree.getEvents(currentRow.value + 1);
                $scope.events = events;
                $scope.initTimeline(events);

                // Select last event
                $scope.showEvent($scope.events[$scope.events.length - 1], $scope.events.length - 1);
            });

            $scope.initTimeline = function(events) {
                // Generate timeline
                smts.timeline.clear();
                smts.timeline.make(events);

                $('.smts-timeline-dash').mouseenter(function() {
                    $(this).addClass('hover');
                });

                $('.smts-timeline-dash').mouseleave(function() {
                    $(this).removeClass('hover');
                });

                $('.smts-timeline-dash').click(function() {
                    let eventId = $(this).attr('data-event');
                    smts.timeline.selectEvent(eventId);


                    let eventRow = smts.tables.events.getRows(`[data-event="${eventId}"]`)[0];
                    if (eventRow) {
                        // Simulate event click to rebuild the tree
                        eventRow.click();

                        // Scroll event table to selected event
                        let eventsTableContainer = document.getElementById('smts-events-table-container');
                        if (eventsTableContainer) {
                            $('#smts-events-table-container').scrollTop(eventRow.offsetTop - eventsTableContainer.offsetTop);
                        }
                    }
                });
            };

            // Show tree up to clicked event
            $scope.showEvent = function(event, index) {
                // Build tree
                currentRow.value = index;
                sharedTree.tree.arrangeTree(currentRow.value);
                generateDomTree(sharedTree.tree, smts.tree.getPosition());

                // Update data table
                smts.tables.data.update(event, `event ${event.event}`);

                // Update timeline
                smts.timeline.update(event);

                // Update events and instances tables
                // N.B.: Angular doesn't offer a `ready` functionality, so it
                // is impossible to detect when the DOM elements are actually
                // ready, thus the functions are called asyncronously.
                window.setTimeout(function() {
                    smts.tables.events.highlight([event]);
                    smts.tables.events.update(sharedTree.tree.selectedNodes);
                    smts.tables.solvers.update(sharedTree.tree.selectedNodes);
                }, 0);

                // Notify selected element
                sharedService.broadcastSelectEvent();
            };

            // Show all events in events table
            $scope.showAll = function() {
                smts.tables.events.showAll();
            };

            // Show only events in events table related to selected nodes
            $scope.showSelected = function() {
                smts.tables.events.showSelected(sharedTree.tree.selectedNodes);
            };
        }]);