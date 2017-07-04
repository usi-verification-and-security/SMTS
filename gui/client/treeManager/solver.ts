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
    }
}