

export { Node };

class Node {
    name: number[]; // es. [0,3]
    type: string; // AND or OR
    children: Node[] = [];
    solvers: string[] = []; // solvers working on it
    status: string; // initially "unknown"

    constructor(name,type:string) {
        this.name = name;
        this.type = type;
        this.status = "unknown";

    }
    test() {

    }
    getName() {
        console.log(this.name);

    }
    addChild(child: Node){
        this.children.push(child);
    }
};
