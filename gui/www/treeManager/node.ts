module TreeManager {
    export class Node {
        name: number[];                          // Path that identifies the node, e.g. [0,3,4,0,1]
        type: string;                            // 'AND' or 'OR'
        children: Node[]            = [];        // Children nodes
        solvers: string[]           = [];        // Solvers working on node
        status: string              = "unknown"; // 'sat', 'unsat' or 'unknown'
        info: object                = {};        // Any kind of information concerning the node
        isStatusPropagated: boolean = false;     // true if status has been propagated from children


        //
        constructor(name: number[], type: string) {
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
            console.log(nodeName);
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


        // Update the node with the event data
        update(event) {
            let node: Node;

            switch (event.event) {
                case 'OR':
                case 'AND':
                    if (event.data && event.data.node) {
                        // Insert node in correct position
                        node = new Node(JSON.parse(event.data.node), event.event);
                        let parent = this.getNode(node.name.slice(0, node.name.length - 1));
                        parent.children[node.name[node.name.length - 1]] = node;
                    }
                    break;

                case '+':
                    node = this.getNode(event.node);
                    node.solvers.push(event.solver);
                    break;

                case '-':
                    node = this.getNode(event.node);
                    let index = node.solvers.indexOf(event.solver);
                    if (index > -1) {
                        node.solvers.splice(index, 1);
                    }
                    break;

                case 'STATUS':
                    node = this.getNode(event.node);
                    node.status = event.data.report;
                    break;

                case 'SOLVED':
                    if (event.data && event.data.node) {
                        node = this.getNode(JSON.parse(event.data.node));
                        if (node.status === 'unknown') {
                            if (!node.isStatusPropagated) {
                                node.status = event.data.status;
                                node.isStatusPropagated = true;
                            }
                            else {
                                console.log(`ERROR: status propagated for node ${node.name} already solved`);
                            }
                        }
                    }
                    break;
            }

            // Update node info
            if (node) node.setInfo(event);
        }

        setInfo(event: Event) {
            if (event.data) {
                for (let key in event.data) {
                    if (key !== 'node' && key !== 'name' && key !== 'report' && key !== 'solver') {
                        this.info[key] = event.data[key];
                    }
                }
            }
        }
    }
}
