

export { Event };

class Event {
    id: number;
    ts: number;
    name: string;
    node: string;
    event: string;
    solver: string; // should become [string, number]
    data: any;
    children: number[] = [];

    constructor(value) {
        this.id = value.id;
        this.ts = value.ts;
        this.name = value.name;
        this.node = value.node;
        this.event = value.event;
        this.solver = value.solver;
        this.data = value.data;

    }

};
