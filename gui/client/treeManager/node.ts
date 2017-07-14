module TreeManager {
    export class Node {
        name: number[];                      // Path that identifies the node, e.g. [0,3,4,0,1]
        type: string;                        // 'AND' or 'OR'
        children: Node[] = [];               // Children nodes
        solvers: string[] = [];              // Solvers working on node
        status: string = "unknown";          // 'sat', 'unsat' or 'unknown'
        isStatusPropagated: boolean = false; // Tell if the status 'sat' or 'unsat' has been propagated from children


        //
        constructor(name, type: string) {
            this.name = name;
            this.type = type;
        }


        // Get height of the tree
        getHeight() {
            let depthMax = 0;
            if (this.children) {
                for (let child of this.children) {
                    let depthChild = child.getHeight() + 1;
                    depthMax = depthChild > depthMax ? depthChild : depthMax;
                }
            }
            return depthMax;
        }


        // Get ratio between the average height and the maximum height of the children nodes
        getBalanceness() {
            if (!this.children || this.children.length === 0) {
                return 1;
            }

            let heightSum = 0, heightMax = 0;
            for (let child of this.children) {
                let heightChild = child.getHeight() + 1;
                heightSum += heightChild;
                if (heightChild > heightMax) {
                    heightMax = heightChild;
                }
            }
            let heightAvg = heightSum / this.children.length;
            return heightAvg / heightMax;
        }


        // Get the max length of a label of all nodes in the given tree
        getMaxLabelLength() {
            let labelLengthMax = this.name.length;
            if (this.children) {
                for (let child of this.children) {
                    let labelLengthChild = child.getMaxLabelLength();
                    labelLengthMax = labelLengthChild > labelLengthMax ? labelLengthChild : labelLengthMax;
                }
            }
            return labelLengthMax;
        }


        // Get node in tree with corresponding node name
        getNode(nodeName) {
            let node: Node = this;
            for (let i = 0; i < nodeName.length; ++i) {
                node = node.children[nodeName[i]];
            }
            return node;
        }

        equalAny(nodes) {
            function nameEqual(node1, node2) {
                if (node1.length !== node2.length) {
                    return false;
                }
                for (let i = 0; i < node1.length; ++i) {
                    if (node1[i] !== node2[i]) {
                        return false;
                    }
                }
                return true;
            }

            for (let node of nodes) {
                if (nameEqual(this.name, node.name)) {
                    return true;
                }
            }
            return false;
        }


        // Update the tree, given an event
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
