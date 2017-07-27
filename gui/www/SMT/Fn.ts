module SMT {
    export class Fn {
        name: string;
        ret: string;

        constructor(name: string, ret: string) {
            this.name = name;
            this.ret = ret;
        }
    }
}