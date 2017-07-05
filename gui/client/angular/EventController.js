app.controller(
    'EventController',
    ['$scope', '$rootScope', 'currentRow', 'sharedTree', '$window', '$http', 'sharedService',
        function ($scope, $rootScope, currentRow, sharedTree, $window, $http, sharedService) {

            $scope.$on('handleBroadcast', function () { // This is called when an instance is selected
                let eventEntries = sharedTree.tree.getEvents(currentRow.value);
                $scope.events = eventEntries;
                $scope.initTimeline(eventEntries); // Initialize timeline
            });

            $scope.initTimeline = function (events) {
                clearTimeline();
                allEvents = events;
                makeTimeline();

                $(".dash").mouseenter(function () {
                    $(this).addClass("hover");
                });

                $(".dash").mouseleave(function () {
                    $(this).removeClass("hover");
                });

                $(".dash").click(function () {
                    let spanNum = $(this).attr("id");
                    selectEvent(spanNum);

                    //Simulate event click to rebuild the tree
                    let find = "event" + spanNum;
                    let row = document.getElementById(find);
                    row.click();

                    // Scroll event table to selected event
                    let query = "#" + find;
                    d5.scrollTop = 0;
                    $('#d5').scrollTop($(query).offset().top - $('#d5').offset().top)
                });
            };

            // Show tree up to clicked event
            $scope.showEvent = function ($event, $index, x) {
                // Highlight selected event
                $('.event-container table tr').removeClass("highlight");
                let query = '.event-container table tr[data-event="' + x.id + '"]';
                $(query).addClass("highlight");

                currentRow.value = $index;
                sharedTree.tree.arrangeTree(currentRow.value);
                let treeView = sharedTree.tree.getTreeView();
                let position = $('g')[0].getAttribute("transform");
                getTreeJson(treeView, position);

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
                let tableName = "Event " + x.event;
                document.getElementById('d6_1').innerHTML = tableName.bold();
                let item = document.getElementById('d6_2');

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