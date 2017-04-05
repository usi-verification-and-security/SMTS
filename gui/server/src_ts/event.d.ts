export { Event };
declare class Event {
    id: number;
    ts: number;
    name: string;
    node: string;
    event: string;
    solver: string;
    data: any;
    children: number[];
    constructor(value: any);
}
