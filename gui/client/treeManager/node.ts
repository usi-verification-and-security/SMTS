module TreeManager {
    export class Node {
        name: number[]; // es. [0,3]
        type: string;   // 'AND', 'OR', '+', '-', 'STATUS' or 'SOLVED'
        children: Node[] = [];
        solvers: string[] = []; // solvers working on it
        status: string = "unknown"; // initially "unknown"

        constructor(name, type: string) {
            this.name = name;
            this.type = type;
            // this.status = "unknown";
        }

        getNode(nodeName) {
            let node: Node = this;
            for (let i = 0; i < nodeName.length; ++i) {
                node = node.children[nodeName[i]];
            }
            return node;
        }

        insertNode(child: Node) {
            let node: Node = this.getNode(child.name.slice(0, child.name.length - 1));
            node.children[child.name[child.name.length - 1]] = child;
        }

        update(event) {
            let node: Node = this.getNode(event.node);

            switch (event.event) {
                case 'OR':
                case 'AND':
                    this.insertNode(new Node(JSON.parse(event.data.node), event.event));
                    break;

                case '+':
                    node.solvers.push(event.solver);
                    break;

                case '-':
                    let index = node.solvers.indexOf(event.solver);
                    if (index > -1) {
                        node.solvers.splice(index, 1);
                    }
                    break;

                case 'STATUS':
                    node.status = event.data.report;
                    break;

                case 'SOLVED':
                    console.log(event.data.status);
                    node.status = event.data.status;
                    this.status = event.data.status;
                    break;
            }
        }
    }
}