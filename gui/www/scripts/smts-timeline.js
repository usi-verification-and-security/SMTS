smts.timeline = {

    events: [],

    clear: function() {
        let line = document.getElementById('smts-timeline-line');
        if (line) line.innerHTML = '';
    },

    getEventIdBytTime: function(time) {
        for (let i = 0; i < this.events.length; i++) {
            if (this.events[i].ts === time) {
                return this.events[i].id;
            }
        }
        return null;
    },

    make: function(events) {
        this.trimEvents(events);

        let line = document.getElementById('smts-timeline-line');
        if (!line) return;

        // Hide timeline if there's less than two events
        if (this.events.length < 2) {
            line.classList.add('smts-hidden');
        }
        else {
            // Show timeline in case it was hidden before
            line.classList.remove('smts-hidden');

            let lineLength = 1 / this.events[this.events.length - 1].ts;

            for (let event of this.events) {
                // Make dash
                let dash = document.createElement('div');
                dash.id = `smts-timeline-event-${event.id}`;
                dash.classList.add('smts-timeline-dash');
                dash.setAttribute('data-event', event.id);
                dash.style.left = `${lineLength * event.ts * 100}%`;

                // Make popup span
                let dashSeconds = document.createElement('div');
                dashSeconds.classList.add('smts-timeline-dash-seconds');
                dashSeconds.classList.add('smts-hidden');
                dashSeconds.innerText = `${event.ts}s`;

                // Set events
                dash.addEventListener('mouseenter', function() {
                    dashSeconds.classList.remove('smts-hidden');
                });

                dash.addEventListener('mouseleave', function() {
                    dashSeconds.classList.add('smts-hidden');
                });

                dash.addEventListener('click', function() {
                    smts.timeline.selectEvent(event.id);

                    let eventRow = smts.tables.events.getRows(`[data-event="${event.id}"]`)[0];
                    if (eventRow) {
                        // Simulate event click to rebuild the tree
                        eventRow.click();
                        smts.tables.events.scroll(event);
                    }
                });

                // Insert elements
                dash.appendChild(dashSeconds);
                line.appendChild(dash);
            }
        }

        // Select first dash
        let firstDash = line.children[0];
        if (firstDash) {
            firstDash.classList.add('smts-active');
        }
    },

    selectEvent: function(eventId) {
        // Unselect previously selected element
        let activeEvents = document.querySelectorAll('.smts-timeline-dash.smts-active');
        for (let activeEvent of activeEvents) {
            activeEvent.classList.remove('smts-active');
        }

        // Get element and select it
        let event = document.getElementById(`smts-timeline-event-${eventId}`);
        if (event) {
            event.classList.add('smts-active');
        }
    },

    // Trim events to avoid having overlapping elements on timeline
    // This only needed to have lighter timeline.
    trimEvents: function(events) {
        this.events = [];
        this.events.push(events[0]);
        for (let i = 1; i < events.length - 1; i++) {
            // Push event only if time is different from last inserted
            if (this.events[this.events.length - 1].ts !== events[i].ts) {
                this.events.push(events[i]);
            }
        }
    },

    update: function(event) {
        let dash = document.getElementById(`smts-timeline-event-${event.id}`);
        if (dash !== null) {
            this.selectEvent(event.id);
        }
        else if (this.events.length > 1) {
            this.selectEvent(this.getEventIdBytTime(event.ts));
        }
    }

};