module SMT {
    export class Fn {
        name: string;
        ret: Type;

        constructor(name: string, ret: Type) {
            this.name = name;
            this.ret = ret;
        }
    }
}