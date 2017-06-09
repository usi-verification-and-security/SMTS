

app.controller('EventController',['$scope','$rootScope','currentRow','sharedTree','$window','$http', 'sharedService',function($scope,$rootScope, currentRow,sharedTree,$window,$http,sharedService){

    $scope.$on('handleBroadcast', function() { // This is called when an instance is selected
        var eventEntries = sharedTree.tree.getEvents(currentRow.value);
        // console.log(eventEntries)
        $scope.entries = eventEntries;
        $scope.initTimeline(eventEntries); // Initialize timeline

    });

    $scope.initTimeline = function(events){
        clearTimeline();
        allEvents = events;
        makeCircles();

        $(".dash").mouseenter(function() {
            $(this).addClass("hover");
        });

        $(".dash").mouseleave(function() {
            $(this).removeClass("hover");
        });

        $(".dash").click(function() {
            var spanNum = $(this).attr("id");
            selectEvent(spanNum);

            //Simulate event click to rebuild the tree
            var find = "event" + spanNum;
            var row = document.getElementById(find);
            row.click();

            // Scroll event table to selected event
            var query = "#" + find;
            d5.scrollTop = 0;
            $('#d5').scrollTop($(query).offset().top - $('#d5').offset().top)
        });
    };

    // Show tree up to clicked event
    $scope.showEvent = function($event,$index,x){
        // Highlight selected event
        $('.event-container table tr').removeClass("highlight");
        var query = '.event-container table tr[data-event="' + x.id +'"]';
        $(query).addClass("highlight");

        currentRow.value = $index;
        sharedTree.tree.arrangeTree(currentRow.value);
        var treeView = sharedTree.tree.getTreeView();
        var position = $('g')[0].getAttribute("transform");
        getTreeJson(treeView, position);

        // Show event's data in dataView
        var object = JSON.parse(x.data);
        if(object == null){
            object = {};
        }
        //sort object by key values
        var keys = [], k, i, len;
        var sortedObj = {};
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
        var ppTable = prettyPrint(sortedObj);
        var tableName = "Event " + x.event;
        document.getElementById('d6_1').innerHTML = tableName.bold();
        var item = document.getElementById('d6_2');

        if(item.childNodes[0]){
            item.replaceChild(ppTable, item.childNodes[0]); //Replace existing table
        }
        else{
            item.appendChild(ppTable);
        }

        //Update timeline
        var circle = document.getElementById(x.id);
        if(circle != null){
            selectEvent(x.id);
        }
        else if((circle == null) && (timelineEvents.length > 1)){ // if timelineEvents.length < 2 then there isn' the timeline
            var index = findElwithSameTS(x.ts);
            selectEvent(index);
        }

        sharedService.broadcastItem2();
    }

}])