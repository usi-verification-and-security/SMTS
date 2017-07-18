smts.timeline = {

    events: [],

    clear: function() {
        let line = document.getElementById('smts-timeline-line');
        if (line) {
            line.innerHTML = '';
        }
    },

    getEventIdBytTime: function(time) {
        for (let i = 0; i < this.events.length - 1; i++) { // TODO: remove `- 1`
            if (this.events[i].ts === time) {
                return this.events[i].id;
            }
        }
        return null;
    },

    make: function(events) {
        this.trimEvents(events);

        let line = document.getElementById('smts-timeline-line');
        if (!line) {
            return; // TODO: create line
        }

        // Hide timeline if there's less than two events
        if (this.events.length < 2) {
            line.classList.add('smts-hidden');
        }
        else {
            let lineLength = 1 / this.events[this.events.length - 1].ts;

            for (let i = 0; i < this.events.length; ++i) {
                // Make dash
                let dash = document.createElement('div');
                // TODO: make prefix for element id
                dash.id = `smts-timeline-event-${this.events[i].id}`;
                dash.classList.add('smts-timeline-dash');
                dash.setAttribute('data-event', this.events[i].id);
                dash.style.left = `${lineLength * this.events[i].ts * 100}%`;

                // Make popup span
                let popupSpan = document.createElement('div');
                popupSpan.classList.add('smts-timeline-popupSpan');
                popupSpan.innerText = `${this.events[i].ts} s`;

                // Insert elements
                dash.append(popupSpan);
                line.append(dash);
            }
        }

        // Select first dash
        let firstDash = line.children[0];
        if (firstDash) {
            firstDash.classList.add('smts-timeline-active');
        }
    },

    selectEvent: function(eventId) {
        // Unselect previously selected element
        let activeEvents = document.querySelectorAll('.smts-timeline-active');
        for (let activeEvent of activeEvents) {
            activeEvent.classList.remove('smts-timeline-active');
        }

        // Get element and select it
        let event = document.getElementById(`smts-timeline-event-${eventId}`);
        if (event) {
            event.classList.add('smts-timeline-active');
        }
    },

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