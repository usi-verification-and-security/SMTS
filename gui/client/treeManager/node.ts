module TreeManager {
    export class Node {
        name: number[]; // es. [0,3]
        type: string; // AND or OR
        children: Node[] = [];
        solvers: string[] = []; // solvers working on it
        status: string = "unknown"; // initially "unknown"

        constructor(name, type: string) {
            this.name = name;
            this.type = type;
            // this.status = "unknown";
        }

        getName() {
            console.log(this.name);
        }

        addChild(child: Node) {
            this.children.push(child);
        }

        insertNode(child: Node) {
            let node: Node = this;
            for (let i = 0; i < child.name.length - 1; ++i) {
                node = node.children[child.name[i]];
            }
            node.children[child.name[child.name.length - 1]] = child;
        }
    }
}