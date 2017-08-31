module TreeManager {
    export class Solver {
        name:  string;   // TODO: change to `address`
        node:  number[]; // TODO: change to `nodeName`
        data:  any;      // Extra information concerning the solving
        event: Event;    // Event currently associated to solver

        constructor(name: string) {
            this.name = name;
            this.node = [];
        }

        update(event: Event) {
            this.data  = event ? event.data : null;
            this.node  = event ? event.node : null;
            this.event = event;
        }
    }
}