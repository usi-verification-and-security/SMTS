export { Tree };
import { Node } from "./Node";
import { Event } from "./Event";
declare class Tree {
    events: Event[];
    solver: Array<[string, string]>;
    solvers: string[];
    treeView: Node;
    constructor();
    createEvents(array: Array<Object>): void;
    arrangeTree(howMany: any): void;
    insertNode(obj: any, parent: any, child: any): any;
    updateNode(obj: any, node: any, event: any, data: any): any;
    rootSolved(obj: any, data: any): any;
    getTreeView(): Node;
    getSolvers(): any;
    assignSolvers(x: number, y: number): void;
    getEvents(x: number): any[];
}
