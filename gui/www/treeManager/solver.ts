module TreeManager {
    export class Solver {
        name: string;
        node: number[]; // node on which the solver is working
        data: Object;

        constructor(name: string) {
            this.name = name;
            this.node = [];
        }

        setData(data: Object) {
            this.data = data;
        }

        setNode(node) {
            this.node = node;
        }

        equalAny(solvers) {
            for (let solver of solvers) {
                if (this.name === solver.name) {
                    return true;
                }
            }
            return false;
        }
    }
}