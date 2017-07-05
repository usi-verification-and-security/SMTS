let allEvents;
let timelineEvents;


function refactorEvents(allEvents) {
    timelineEvents = [];
    timelineEvents.push(allEvents[0]);
    let counter = 0;
    for (let i = 1; i < allEvents.length - 1; i++) {
        if (timelineEvents[counter].ts !== allEvents[i].ts) {
            timelineEvents.push(allEvents[i]);
            counter++;
        }
    }
}

// This function returns the element in timelineEvents with the same ts as the selected element
function findElwithSameTS(time) {
    for (let i = 0; i < timelineEvents.length - 1; i++) {
        if (timelineEvents[i].ts === time) {
            return timelineEvents[i].id;
        }
    }
}

//Main function.
function makeTimeline() {
    // If in allEvents the are more events with the sam timeStamp just the first one will be left
    refactorEvents(allEvents);

    //Forget the timeline if there's only one event
    if (timelineEvents.length < 2) {
        $("#line").hide();

        //This is what you really want.
    } else if (timelineEvents.length >= 2) {
        let relativeInt;
        let lineLength = 1 / timelineEvents[timelineEvents.length - 1].ts;

        //Draw first line
        $("#line").append(`<div class="dash" id="${timelineEvents[0].id}" style="left: 0;"><div class="popupSpan">${timelineEvents[0].ts} s</div></div>`);

        //Loop through middle lines
        for (let i = 1; i < timelineEvents.length - 1; i++) {
            relativeInt = lineLength * timelineEvents[i].ts * 100;

            //Draw the line
            $("#line").append(`<div class="dash" id="${timelineEvents[i].id}" style="left: ${relativeInt}%;"><div class="popupSpan">${timelineEvents[i].ts} s</div></div>`);
        }

        relativeInt = lineLength * timelineEvents[timelineEvents.length - 1].ts * 100;
        //Draw the last line
        $("#line").append(`<div class="dash" id="${timelineEvents[timelineEvents.length - 1].id}" style="left: ${relativeInt}%;"><div class="popupSpan">${timelineEvents[timelineEvents.length - 1].ts} s</div></div>`);
    }

    $(".line:first").addClass("active");
}

function selectEvent(selector) {
    $selector = "#" + selector;
    $spanSelector = $selector.replace("dash", "span");

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
    }
}

function clearTimeline() {
    let el = document.getElementById('line');
    if (el.childNodes.length !== 0) {
        $(el).empty();
    }
}