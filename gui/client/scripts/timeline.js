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
        $("#smts-timeline-line").hide();

        //This is what you really want.
    } else if (timelineEvents.length >= 2) {
        let relativeInt;
        let lineLength = 1 / timelineEvents[timelineEvents.length - 1].ts;

        //Draw first line
        $("#smts-timeline-line").append(`<div class="smts-timeline-dash" id="${timelineEvents[0].id}" style="left: 0;"><div class="smts-timeline-popupSpan">${timelineEvents[0].ts} s</div></div>`);

        //Loop through middle lines
        for (let i = 1; i < timelineEvents.length - 1; i++) {
            relativeInt = lineLength * timelineEvents[i].ts * 100;

            //Draw the line
            $("#smts-timeline-line").append(`<div class="smts-timeline-dash" id="${timelineEvents[i].id}" style="left: ${relativeInt}%;"><div class="smts-timeline-popupSpan">${timelineEvents[i].ts} s</div></div>`);
        }

        relativeInt = lineLength * timelineEvents[timelineEvents.length - 1].ts * 100;
        //Draw the last line
        $("#smts-timeline-line").append(`<div class="smts-timeline-dash" id="${timelineEvents[timelineEvents.length - 1].id}" style="left: ${relativeInt}%;"><div class="smts-timeline-popupSpan">${timelineEvents[timelineEvents.length - 1].ts} s</div></div>`);
    }

    $(".smts-timeline-line:first").addClass("smts-timeline-active");
}

function selectEvent(selector) {
    $selector = "#" + selector;
    $spanSelector = $selector.replace("smts-timeline-dash", "smts-timeline-span");

    $(".smts-timeline-active").removeClass("smts-timeline-active");
    $($selector).addClass("smts-timeline-active");

    if ($($spanSelector).hasClass("smts-timeline-right")) {
        $(".smts-timeline-center").removeClass("smts-timeline-center").addClass("smts-timeline-left")
        $($spanSelector).addClass("smts-timeline-center");
        $($spanSelector).removeClass("smts-timeline-right")
    } else if ($($spanSelector).hasClass("smts-timeline-left")) {
        $(".smts-timeline-center").removeClass("center").addClass("smts-timeline-right");
        $($spanSelector).addClass("smts-timeline-center");
        $($spanSelector).removeClass("smts-timeline-left");
    }
}

function clearTimeline() {
    let el = document.getElementById('smts-timeline-line');
    if (el.childNodes.length !== 0) {
        $(el).empty();
    }
}