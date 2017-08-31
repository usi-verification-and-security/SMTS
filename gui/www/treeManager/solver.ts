module TreeManager {
    export class Solver {
        name: string;   // TODO: change to `address`
        node: number[]; // TODO: change to `nodeName`
        data: Object;
        event: Event;

        constructor(name: string) {
            this.name = name;
            this.node = [];
        }

        setData(data: Object) {
            this.data = data;
        }

        setNode(node) {
            this.node = node;
        }

        setEvent(event: Event) {
            this.event = event;
        }

        update(event: Event) {
            this.data = event ? event.data : null;
            this.node = event ? event.node : null;
            this.event = event;
        }

        equalAny(solvers) {
            for (let solver of solvers) {
                if (this.name === solver.name) {
                    return true;
                }
            }
            return false;
        }
    }
}