module TreeManager {
    export class Node {
        name:               number[];             // Path that identifies the node, e.g. [0,3,4,0,1]
        type:               string;               // 'AND' or 'OR'
        children:           Node[]   = [];        // Children nodes
        solvers:            string[] = [];        // Solvers working on node
        status:             string   = 'unknown'; // 'sat', 'unsat' or 'unknown'
        info:               object   = {};        // Information concerning the node
        isStatusPropagated: boolean  = false;     // `true` if status has been propagated from children

        // Constructor
        // @param {number[]} path: Path of the node.
        // @param {string} tye: Type of the node.
        constructor(path: number[], type: string) {
            this.name = path;
            this.type = type;
        }

        // Tell whether a given list of nodes contains this node
        // @param {Node[]} nodes: List of nodes that may or may not contain
        // this node.
        // @return {boolean}: `true` if `nodes` contain `this`, `false`
        // otherwise.
        equalAny(nodes: Node[]) : boolean {
            return nodes.some(node => {
                if (this.name.length !== node.name.length) {
                    return false;
                }
                for (let i = 0; i < this.name.length; ++i) {
                    if (this.name[i] !== node.name[i]) {
                        return false;
                    }
                }
                return true;
            });
        }

        // Get balanceness of tree starting from this node
        // The balanceness is the ratio between average height and maximum
        // height of children nodes.
        // @return {number}: Balanceness of the node.
        getBalanceness() : number {
            if (!this.children || this.children.length === 0) {
                return 1;
            }

            let heightSum: number = 0, heightMax: number = 0;
            for (let child of this.children) {
                let heightChild: number = child.getHeight() + 1;
                heightSum += heightChild;
                if (heightChild > heightMax) {
                    heightMax = heightChild;
                }
            }
            let heightAvg: number = heightSum / this.children.length;
            return heightAvg / heightMax;
        }

        // Get height of the tree starting from this node
        // @return {number}: Height of the tree.
        getHeight() : number {
            let depthMax: number = 0;
            if (this.children) {
                for (let child of this.children) {
                    let depthChild: number = child.getHeight() + 1;
                    depthMax = depthChild > depthMax ? depthChild : depthMax;
                }
            }
            return depthMax;
        }

        // Get node in tree with corresponding node path
        // @param {number[]} path: Path of the node.
        // @return {Node}: The node with the given path.
        getNode(path: number[]) : Node {
            let node: Node = this;
            for (let i = 0; i < path.length; ++i) {
                node = node.children[path[i]];
            }
            return node;
        }

        // Set node information, taken from an event
        // All data present in `event.data` is copied to the node, with the
        // exception of `node`, `name`, `report` and `solver` data.
        // @param {Event} event: The event from which the data is taken from.
        setInfo(event: Event) : void {
            if (event.data) {
                for (let key in event.data) {
                    if (key !== 'node' && key !== 'name' && key !== 'report' && key !== 'solver') {
                        this.info[key] = event.data[key];
                    }
                }
            }
        }

        // Update tree starting from this node with the event data
        // @params {Event} event: Event that contains information to update the
        // tree.
        update(event: Event) : void {
            let node: Node;

            switch (event.type) {
                case 'OR':
                case 'AND':
                    if (event.data && event.data.node) {
                        // Insert node in correct position
                        node = new Node(JSON.parse(event.data.node), event.type);
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
    }
}
