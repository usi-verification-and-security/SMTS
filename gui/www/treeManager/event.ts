module TreeManager {
    export class Event {
        id: number;
        ts: number;
        name: string;
        node: number[];
        event: string;
        solver: string;
        data: any;

        constructor(value) {
            this.id = value.id;
            this.ts = value.ts;
            this.name = value.name;
            this.node = value.node;
            this.event = value.event;
            this.solver = value.solver;
            this.data = value.data;

            // Remove all spaces from string array
            if (this.data && this.data.node) {
                this.data.node = this.data.node.replace(/\s/g, '');
            }
        }

        getMainNode() {
            if (this.data && this.data.node) {
                return JSON.parse(this.data.node);
            }
            return this.node;
        }

        setTs(value: number) {
            this.ts = value;
        }

        equalAny(events) {
            for (let event of events) {
                if (this.id === event.id) {
                    return true;
                }
            }
            return false;
        }

    }
}