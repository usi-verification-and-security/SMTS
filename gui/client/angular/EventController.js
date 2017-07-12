app.controller(
    'EventController',
    ['$scope', '$rootScope', 'currentRow', 'sharedTree', '$window', '$http', 'sharedService',
        function ($scope, $rootScope, currentRow, sharedTree, $window, $http, sharedService) {

            $scope.$on('handleBroadcast', function () { // This is called when an instance is selected
                let eventEntries = sharedTree.tree.getEvents(currentRow.value + 1);
                $scope.events = eventEntries;
                $scope.initTimeline(eventEntries); // Initialize timeline
            });

            $scope.initTimeline = function (events) {
                clearTimeline();
                allEvents = events;
                makeTimeline();

                $(".smts-timeline-dash").mouseenter(function () {
                    $(this).addClass("hover");
                });

                $(".smts-timeline-dash").mouseleave(function () {
                    $(this).removeClass("hover");
                });

                $(".smts-timeline-dash").click(function () {
                    let spanNum = $(this).attr("id");
                    selectEvent(spanNum);

                    //Simulate event click to rebuild the tree
                    let find = "event" + spanNum;
                    let row = document.getElementById(find);
                    row.click();

                    // Scroll event table to selected event
                    let query = "#" + find;
                    $('#smts-event-container').scrollTop($(query).offset().top - $('#smts-event-container').offset().top)
                });
            };

            // Show tree up to clicked event
            $scope.showEvent = function ($event, $index, x) {
                // Highlight selected event
                $('#smts-event-container table tr').removeClass("smts-highlight");
                let query = '#smts-event-container table tr[data-event="' + x.id + '"]';
                $(query).addClass("smts-highlight");

                currentRow.value = $index;
                sharedTree.tree.arrangeTree(currentRow.value);
                let root = sharedTree.tree.getRoot();
                let position = $('g')[0].getAttribute("transform");
                generateDomTree(root, sharedTree.tree.getSelectedNodeNames(currentRow.value), position);

                // Show event's data in dataView
                let object = x.data;
                if (object === null) {
                    object = {};
                }

                //sort object by key values
                let keys = [], k, i, len;
                let sortedObj = {};
                for (k in object) {
                    if (object.hasOwnProperty(k)) {
                        keys.push(k);
                    }
                }
                keys.sort();
                len = keys.length;
                for (i = 0; i < len; i++) {
                    k = keys[i];
                    sortedObj[k] = object[k];
                }
                let ppTable = prettyPrint(sortedObj);
                let tableName = "EVENT " + x.event;
                document.getElementById('smts-data-container-title').innerHTML = tableName.bold();
                let item = document.getElementById('smts-data-container-content');

                if (item.childNodes[0]) {
                    item.replaceChild(ppTable, item.childNodes[0]); //Replace existing table
                }
                else {
                    item.appendChild(ppTable);
                }

                //Update timeline
                let circle = document.getElementById(x.id);
                if (circle !== null) {
                    selectEvent(x.id);
                }
                else if ((circle === null) && (timelineEvents.length > 1)) { // if timelineEvents.length < 2 then there isn' the timeline
                    let index = findElwithSameTS(x.ts);
                    selectEvent(index);
                }

                sharedService.broadcastItem2();
            }
        }]);