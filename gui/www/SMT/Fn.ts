module SMT {
    export class Fn {
        name: string;
        args: Type[];
        ret: Type;

        constructor(name: string, args: Type[], ret: Type) {
            this.name = name;
            this.args = args;
            this.ret = ret;
        }
    }
}