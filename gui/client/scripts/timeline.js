
var allEvents = undefined;
var timelineEvents;


function refactorEvents(allEvents) {
    timelineEvents = [];
    timelineEvents.push(allEvents[0]);
    var counter = 0;
    for (var i = 1; i < allEvents.length - 1; i++) {
        if(timelineEvents[counter].ts != allEvents[i].ts){
            timelineEvents.push(allEvents[i]);
            counter++;
        }
    }
}

// This function returns the element in timelineEvents with the same ts as the selected element
function findElwithSameTS(time){
    for (var i = 0; i < timelineEvents.length - 1; i++) {
        if(timelineEvents[i].ts == time){
            return timelineEvents[i].id;
        }
    }
}

//Main function. Draw your circles.
function makeCircles() {
    // If in allEvents the are more events with the sam timeStamp just the first one will be left
    refactorEvents(allEvents);

    //Forget the timeline if there's only one date.
    if (timelineEvents.length < 2) {
        $("#line").hide();

        //This is what you really want.
    } else if (timelineEvents.length >= 2) {
        var relativeInt;
        var lineLength = 1 / timelineEvents[timelineEvents.length-1].ts;

        //Draw first date circle
        $("#line").append('<div class="circle" id="' + timelineEvents[0].id + '" style="left: ' + 0 + '%;"><div class="popupSpan">' + timelineEvents[0].ts + ' s' + '</div></div>');

        //Loop through middle circles
        for (i = 1; i < timelineEvents.length - 1; i++) {
            relativeInt = lineLength * timelineEvents[i].ts * 100;

            //Draw the circle
           $("#line").append('<div class="circle" id="' + timelineEvents[i].id + '" style="left: ' + relativeInt + '%;"><div class="popupSpan">' + timelineEvents[i].ts + ' s' + '</div></div>');
        }

        relativeInt = lineLength * timelineEvents[timelineEvents.length-1].ts * 100;
        //Draw the last circle
        $("#line").append('<div class="circle" id="' + timelineEvents[timelineEvents.length-1].id + '" style="left: ' + relativeInt + '%;"><div class="popupSpan">' + timelineEvents[timelineEvents.length-1].ts + ' s' + '</div></div>');
    }

    $(".circle:first").addClass("active");
}

function selectEvent(selector) {
    $selector = "#" + selector;
    $spanSelector = $selector.replace("circle", "span");
    var current = $selector.replace("circle", "");

    $(".active").removeClass("active");
    $($selector).addClass("active");

    if ($($spanSelector).hasClass("right")) {
        $(".center").removeClass("center").addClass("left")
        $($spanSelector).addClass("center");
        $($spanSelector).removeClass("right")
    } else if ($($spanSelector).hasClass("left")) {
        $(".center").removeClass("center").addClass("right");
        $($spanSelector).addClass("center");
        $($spanSelector).removeClass("left");
    };
};

function clearTimeline() {
    var el = document.getElementById('line');
    if(el.childNodes.length != 0){
        $(el).empty();
    }
}