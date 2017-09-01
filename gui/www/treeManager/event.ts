module TreeManager {
    export class Event {
        id:            number;   // Identification number of the event
        ts:            number;   // Time stamp (in seconds), symbolising when the event occurred
        nodeName:      number[]; // Name of the node associated to the event
        type:          string;   // Type of the event ('AND', 'OR', '+', '-', 'SOLVED' or 'STATUS')
        solverAddress: string;   // Address of the solver associated to the event
        data:          any;      // Extra data associated to the event

        // Constructor
        // @param {any} event: Event object that is used as base to build the
        // event.
        // @param {number} startTime [default=0]: Optional starting time. If
        // provided, the starting time is detracted to the event time stamp.
        constructor(event: any, startTime) {
            this.id            = event.id;
            this.ts            = event.ts - startTime;
            this.nodeName      = event.nodeName;
            this.type          = event.type;
            this.solverAddress = event.solverAddress;
            this.data          = event.data;

            // Remove all spaces from string array
            if (this.data && this.data.node) {
                this.data.node = this.data.node.replace(/\s/g, '');
            }
        }

        // Get main node associated to the event
        // The main node of the event is considered to be the node present in
        // the event data, if any, otherwise the actual node.
        // @return {number[]}: The name of the main node.
        getMainNode() : number[] {
            if (this.data && this.data.node) {
                return JSON.parse(this.data.node);
            }
            return this.nodeName;
        }
    }
}