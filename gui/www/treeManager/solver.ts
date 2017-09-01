module TreeManager {
    export class Solver {
        address:  string;    // Address of the solver, e.g.: '["127.0.0.1", 64742]'
        nodePath: number[];  // Path of the node associated to the solver
        data:     any;       // Extra information concerning the solving
        event:    Event;     // Event currently associated to solver

        // Constructor
        // @param {string} address: Address of the solver.
        constructor(address: string) {
            this.address  = address;
            this.nodePath = [];
        }

        // Update solver with given event information
        // @param {Event} event: The event.
        update(event: Event) : void {
            this.data      = event ? event.data     : null;
            this.nodePath  = event ? event.nodeName : null;
            this.event     = event;
        }
    }
}