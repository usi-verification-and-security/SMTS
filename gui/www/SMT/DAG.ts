module SMT {

    export class DAG {

        // The DAG class takes a json smt2 file and converts it into a data
        // structure to then generate the DAG.
        // E.g: The following smt2 code
        //
        //     (assert (< a 10))
        //     (assert (= b (f a b))
        //     (assert (let (x (f a b)) (and x (f a c))))
        //
        // gives the following data structure (`{}` are Nodes, `[...]` the
        // content of the array is defined elsewhere)
        //
        //     this.nodes = {
        //         '10': [],
        //         'a': [],
        //         'b': [],
        //         'c': [],
        //         'f': [
        //             {
        //                 name: 'f', pos: 0, args: [
        //                     {name: 'a', pos: -1, args: []},
        //                     {name: 'b', pos: -1, args: []}
        //                 ]
        //             },
        //             {
        //                 name: 'f', pos: 1, args: [
        //                     {name: 'a', pos: -1, args: []},
        //                     {name: 'c', pos: -1, args: []}
        //                 ]
        //             }
        //         ],
        //         '<': [
        //             {
        //                 name: '<', pos: 0, args: [
        //                     {name: 'a', pos: -1, args: []},
        //                     {name: '10', pos: -1, args: []}
        //                 ]
        //             }
        //         ],
        //         '=': [
        //             {
        //                 name: '=', pos: 0, args: [
        //                     {name: 'b', pos: -1, args: []},
        //                     {name: 'f', pos: 0, args: [...]}
        //                 ]
        //             }
        //         ],
        //         'and': [
        //             {
        //                 name: 'and', pos: 0, args: [
        //                     {name: '<', pos: 0, args: [...]},
        //                     {name: '=', pos: 0, args: [...]},
        //                     {name: 'and', pos: 1, args: [...]},
        //                 ]
        //             },
        //             {
        //                 name: 'and', pos: 1, args: [
        //                     {name: 'f', pos: 0, args: [...]},
        //                     {name: 'f', pos: 1, args: [...]},
        //                 ]
        //             },
        //         ]
        //     };

        // Variables
        root:  Node                        = new Node('and', 0, []);
        nodes: { [name: string]: Node[] }  = {};

        types: { [name: string]: boolean } = {};
        fns:   { [name: string]: Fn }      = {};


        // DAG constructor
        // @param {any[]} smt: Json object representing the smt.
        constructor(smt: any[][]) {
            this.root  = new Node('and', 0, []);
            this.nodes = {'and': [this.root]};

            this.fns   = {};
            this.types = {};

            // Default types and functions
            this.types['Bool'] = true;
            this.types['Real'] = true;

            this.fns['and'] = new Fn('and', 'Bool');
            this.fns['or']  = new Fn('or', 'Bool');
            this.fns['xor'] = new Fn('xor', 'Bool');
            this.fns['not'] = new Fn('not', 'Bool');
            this.fns['=>']  = new Fn('=>', 'Bool');
            this.fns['>']   = new Fn('>', 'Bool');
            this.fns['<']   = new Fn('<', 'Bool');
            this.fns['>=']  = new Fn('>=', 'Bool');
            this.fns['<=']  = new Fn('<=', 'Bool');
            this.fns['=']   = new Fn('=', 'Bool');

            if (smt) {
                this.parse(smt);
            }
        }

        // Parse smt json object to create a DAG data structure
        // @params {any[][]} smt: Json object representing the smt.
        parse(smt: any[][]): void {
            for (let obj of smt) {
                switch (obj[0]) {
                    case 'declare-sort':
                        // `obj` has the following structure
                        //   - obj[0] = 'define-sort'
                        //   - obj[1] = objName
                        this.types[obj[1]] = true;
                        break;

                    case 'declare-fun':
                        // `obj` has the following structure
                        //   - obj[0] = 'define-fun'
                        //   - obj[1] = fnName
                        //   - obj[2] = fnArgName[]
                        if (!this.types[obj[3]]) {
                            console.log(`SMT parse error: Function with non-existing type '${obj[3]}'`);
                            return;
                        }
                        this.fns[obj[1]] = new Fn(obj[1], obj[3]);
                        break;

                    case 'assert':
                        // `obj` has the following structure
                        //   - obj[0] = 'assert'
                        //   - obj[1] = nodeObj
                        this.root.args.push(this.makeNode(obj[1]));
                        break;
                }
            }
        }

        // Make a node
        // @param {any} node: The node can be:
        //   - An object name string
        //     - obj = objName
        //   - A let statement object
        //     - obj[0] = 'let'
        //     - obj[1] = letAlias[] // An array of alias objects
        //     - obj[2] = childObj
        //   - A function object
        //     - obj[0] = fnName
        //     - obj[1]..obj[n] = fnArg
        // @param {any[]} aliases: Aliases for leaf nodes defined by `let`.
        // @return {Node}: Node added to this.nodes.
        makeNode(obj: any, aliases: any[] = []): Node {
            // Leaf node
            if (typeof obj === 'string') {
                return this.makeLeaf(obj, aliases);
            }

            // Obj is a let statement
            if (obj[0] === 'let') {
                aliases = aliases.concat(obj[1]); // Update aliases
                return this.makeNode(obj[2], aliases);
            }

            // Obj in not a let statement
            let args: Node[] = [];
            for (let i: number = 1; i < obj.length; ++i) {
                args.push(this.makeNode(obj[i], aliases));
            }

            // Add obj to this.nodes if not already there
            if (!this.nodes[obj[0]]) {
                this.nodes[obj[0]] = [];
            }

            let nodes: Node[] = this.nodes[obj[0]];

            // Check if object with same name and arguments is already present
            for (let i: number = 0; i < nodes.length; ++i) {
                if (DAG.argsEqual(args, nodes[i].args)) {
                    return nodes[i];
                }
            }

            let node: Node = new Node(obj[0], nodes.length, args);
            nodes.push(node);
            return node;
        }

        // Make leaf node, or make normal node if leaf is aliased
        // @param {string} leafName: Name of the leaf node.
        // @param {any[]} aliases: Let definitions that might match the
        // `leafName`.
        // @return {Node}: Node added to this.nodes.
        makeLeaf(leafName: string, aliases: any[]): Node {
            let index = DAG.indexOf(aliases, leafName);
            if (index !== -1) {
                return this.makeNode(aliases[index][1], aliases);
            }

            if (!this.nodes[leafName]) {
                let node = new Node(leafName, 0, []);
                this.nodes[leafName] = [node];
            }
            return this.nodes[leafName][0];
        }

        // Get type of a function
        // @param {string} fn: Name of the function for which we want the
        // return value type.
        // @return {string}: Return value of `fn`. Numbers are not present in
        // `this.fns`, so it first checks if `fn` is a number, in which case it
        // returns 'Real', else it returns the value in `this.fns`.
        getType(fnName: string) : string {
            let n = Number(fnName);
            if (n || n === 0) {
                return 'Real';
            }
            let fn = this.fns[fnName]
            if (!fn) console.log(`DAG parse error: Function '${fnName}' not found`);
            return fn.ret;
        }

        // Get number of nodes
        // @return {number}: The number of nodes.
        getSize() : number {
            let size: number = 0;
            for (let key in this.nodes) {
                size += this.nodes[key].length;
            }
            return size;
        }

        // Check if all args of two lists are equal
        // Two arguments are considered equal if both `name` and `pos`
        // attributes match.
        // @param {Node[]} args1: List of args to be compared.
        // @param {Node[]} args2: List of args to be compared.
        static argsEqual(args1: Node[], args2: Node[]): boolean {
            if (args1.length !== args2.length) {
                return false;
            }
            for (let i = 0; i < args1.length; ++i) {
                if (args1[i].pos !== args2[i].pos || args1[i].name !== args2[i].name) {
                    return false;
                }
            }
            return true;
        }

        // Get index of `objName` in the `aliases` array
        // @param {object[]} aliases: List of objects, each object has the
        // following structure
        //   - aliases[i][0] = ithObjName
        //   - aliases[i][1] = ithObj
        // @param {string} objName: The object name to be found in the list.
        // @return {number}: Index of objName in aliases, or -1 if not found.
        static indexOf(aliases: object[], objName: string): number {
            for (let i: number = 0; i < aliases.length; ++i) {
                if (aliases[i][0] === objName) {
                    return i;
                }
            }
            return -1;
        }
    }
}