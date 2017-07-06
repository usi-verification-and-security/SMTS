module TreeManager {
    export class Node {
        name: number[];                      // Path that identifies the node, e.g. [0,3,4,0,1]
        type: string;                        // 'AND', 'OR', '+', '-', 'STATUS' or 'SOLVED'
        children: Node[] = [];               // Children nodes
        solvers: string[] = [];              // Solvers working on node
        status: string = "unknown";          // 'sat', 'unsat' or 'unknown'
        isStatusPropagated: boolean = false; // Tell if the status 'sat' or 'unsat' has been propagated from children

        constructor(name, type: string) {
            this.name = name;
            this.type = type;
        }

        getNode(nodeName) {
            let node: Node = this;
            for (let i = 0; i < nodeName.length; ++i) {
                node = node.children[nodeName[i]];
            }
            return node;
        }

        update(event) {
            let node: Node = this.getNode(event.node);

            switch (event.event) {
                case 'OR':
                case 'AND':
                    // Insert node in correct position
                    let child = new Node(JSON.parse(event.data.node), event.event);
                    node = this.getNode(child.name.slice(0, child.name.length - 1));
                    node.children[child.name[child.name.length - 1]] = child;
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
                    if (event.data && event.data.node) {
                        node = this.getNode(event.data.node);
                        if (node.status === 'unknown') {
                            if (!node.isStatusPropagated) {
                                node.status = event.data.report;
                                node.isStatusPropagated = true;
                            }
                            else {
                                console.log(`ERROR: status propagated for node ${node.name} already solved`);
                            }
                        }
                    }
                    break;
            }
        }
    }
}
