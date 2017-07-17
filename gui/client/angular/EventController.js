app.controller(
    'EventController', ['$scope', '$rootScope', 'currentRow', 'sharedTree', '$window', '$http', 'sharedService',
        function($scope, $rootScope, currentRow, sharedTree, $window, $http, sharedService) {

            // Update events on instance selected
            $scope.$on('select-instance', function() {
                let events = sharedTree.tree.getEvents(currentRow.value + 1);
                $scope.events = events;
                $scope.initTimeline(events);
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
                    let dashId = $(this).attr('id');
                    // `dashId` has the form 'smts-events-event-eventId'
                    let eventId = dashId.split('-')[3];
                    smts.timeline.selectEvent(eventId);

                    let eventRow = document.getElementById(`smts-events-event-${dashId}`);
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
                // Highlight selected event
                smts.tables.events.highlight([event]);

                // Rebuild tree
                currentRow.value = index;
                sharedTree.tree.arrangeTree(currentRow.value);
                generateDomTree(sharedTree.tree, smts.tree.getPosition());

                // Update data table
                smts.tables.data.update(event, `event ${event.event}`);

                // Update timeline
                smts.timeline.update(event);

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