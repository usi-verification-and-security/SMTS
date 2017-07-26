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

        argsEqual(other: Node) {
            if (this.args.length !== other.args.length) {
                return false;
            }
            for (let i = 0; i < this.args.length; ++i) {
                if (this.args[i].pos !== other.args[i].pos
                    || this.args[i].name !== other.args[i].name) {
                    return false;
                }
            }
            return true;
        }
    }
}