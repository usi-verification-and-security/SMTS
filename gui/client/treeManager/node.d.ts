export { Node };
declare class Node {
    name: number[];
    type: string;
    children: Node[];
    solvers: string[];
    status: string;
    constructor(name: any, type: string);
    test(): void;
    getName(): void;
    addChild(child: Node): void;
}
