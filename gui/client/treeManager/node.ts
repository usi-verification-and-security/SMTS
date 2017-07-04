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

        updateNode(nodeName, event, data) {
            nodeName = JSON.parse(nodeName);
            let node: Node = this;

            // Get corresponding node in tree
            for (let i = 0; i < nodeName.length; ++i) {
                node = node.children[nodeName[i]];
            }

            switch (event) {
                case '+':
                    node.solvers.push(data);
                    break;

                case '-':
                    let index = node.solvers.indexOf(data);
                    if (index > -1) {
                        node.solvers.splice(index, 1);
                    }
                    break;

                case 'STATUS':
                case 'SOLVED':
                    node.status = data;
                    break;
            }
        }

        setStatus(status) {
            this.status = status;
        }
    }
}