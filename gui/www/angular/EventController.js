app.controller(
    'EventController', ['$scope', '$rootScope', 'currentRow', 'sharedTree', '$window', '$http', 'sharedService',
        function($scope, $rootScope, currentRow, sharedTree, $window, $http, sharedService) {

            // Update events on instance selected
            $scope.$on('select-instance', function() {
                // Make tree
                sharedTree.tree.arrangeTree(currentRow.value);
                generateDomTree(sharedTree.tree, null);

                // Make timeline
                let events = sharedTree.tree.getEvents(currentRow.value + 1);
                $scope.events = events;
                $scope.initTimeline(events);
            });

            // Select last event in table when table is fully populated
            $scope.$on('table-events-populated', function() {
                // TODO: Make it work without timeout
                setTimeout(function() {
                    smts.tables.events.highlight([$scope.events[$scope.events.length - 1]]);
                }, 0);
                // TODO: scroll to end
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
                    smts.timeline.selectEvent($(this).attr('data-event'));

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