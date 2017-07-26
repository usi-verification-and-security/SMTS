module SMT {
    export class Node {
        name: string;
        pos: number;  // Position in the SMT.nodes hashtable
        args: Node[];

        constructor(name: string, pos: number, args: Node[]) {
            this.name = name;
            this.pos = pos;
            this.args = args;
        }
    }
}