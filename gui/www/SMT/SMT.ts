module SMT {
    export class SMT {

        // The SMT class takes a json smt2 file and converts it into a data
        // structure to then generate the DAG.
        // E.g: The following smt2 code
        //   (assert (< a 10))
        //   (assert (= b (f a b))
        //   (assert (let (x (f a b)) (and x (f a c))))
        // gives the following data structure ({} are Nodes)
        //   this.nodes = {
        //     '10': [],
        //     'a': [],
        //     'b': [],
        //     'c': [],
        //     'f': [
        //       {name: 'f', pos: 0, args: [
        //         {name: 'a', pos: -1, args: []},
        //         {name: 'b', pos: -1, args: []}
        //       ]},
        //       {name: 'f', pos: 1, args: [
        //         {name: 'a', pos: -1, args: []},
        //         {name: 'c', pos: -1, args: []}
        //       ]}
        //     ],
        //     '<': [
        //       {name: '<', pos: 0, args: [
        //         {name: 'a', pos: -1, args: []},
        //         {name: '10', pos: -1, args: []}
        //       ]}
        //     ],
        //     '=': [
        //       {name: '=', pos: 0, args: [
        //         {name: 'b', pos: -1, args: []},
        //         {name: 'f', pos: 0, args: [...]}
        //       ]}
        //     ],
        //     'and': [
        //       {name: 'and', pos: 0, args: [
        //         {name: '<', pos: 0, args: [...]},
        //         {name: '=', pos: 0, args: [...]},
        //         {name: 'and', pos: 1, args: [...]},
        //       ]},
        //       {name: 'and', pos: 1, args: [
        //         {name: 'f', pos: 0, args: [...]},
        //         {name: 'f', pos: 1, args: [...]},
        //       ]},
        //     ]

        root: Node; // The root node is 'and'
        nodes: { [name: string]: Node[] };

        fns: { [name: string]: Fn };
        types: { [name: string]: Type };


        constructor(smt: any) {
            this.root = new Node('and', 0, []);
            this.nodes = {'and': [this.root]};

            this.fns = {};
            this.types = {};

            // Default types and functions
            let typeBool = new Type('Bool');
            this.types['Bool'] = typeBool;
            this.fns['and'] = new Fn('and', [typeBool, typeBool], typeBool);
            this.fns['or'] = new Fn('or', [typeBool, typeBool], typeBool);
            this.fns['xor'] = new Fn('xor', [typeBool, typeBool], typeBool);
            this.fns['not'] = new Fn('not', [typeBool], typeBool);
            this.fns['=>'] = new Fn('=>', [typeBool, typeBool], typeBool);

            if (smt) {
                this.make(smt);
            }
        }

        static argsEqual(args1: Node[], args2: Node[]) {
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

        static indexOf(aliases: object[], obj: string) {
            for (let i = 0; i < aliases.length; ++i) {
                if (aliases[i][0] === obj) {
                    return i;
                }
            }
            return -1;
        }

        make(smt: any) {
            for (let obj of smt) {
                switch (obj[0]) {
                    case 'declare-sort':
                        // obj = typeName
                        this.types[obj] = new Type(obj);
                        break;

                    case 'declare-fun':
                        // `obj` has the following structure
                        // - obj[0] = 'define-fun'
                        // - obj[1] = fnName
                        // - obj[2] = fnArgName[]
                        let types = [];
                        for (let type of obj[2]) {
                            types.push(this.types[type]);
                        }
                        this.fns[obj[1]] = new Fn(obj[1], types, this.types[obj[3]]);
                        break;

                    case 'assert':
                        // `obj` has the following structure
                        // - obj[0] = 'assert'
                        // - obj[1] = nodeObj
                        this.root.args.push(this.makeNode(obj[1]));
                        break;
                }
            }
        }

        makeNodeLeaf(leaf: string, aliases: object[]) {
            let index = SMT.indexOf(aliases, leaf);
            if (index !== -1) {
                return this.makeNode(aliases[index][1], aliases);
            }

            if (!this.nodes[leaf]) {
                let node = new Node(leaf, 0, []);
                this.nodes[leaf] = [node];
            }
            return this.nodes[leaf][0];
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
        // @param {object[]} aliases: Aliases for leaf nodes defined by `let`.
        makeNode(obj: any, aliases: object[] = []) {
            // Leaf node
            if (typeof obj === 'string') {
                return this.makeNodeLeaf(obj, aliases);
            }

            // Obj is a let statement
            if (obj[0] === 'let') {
                aliases = aliases.concat(obj[1]); // Update aliases
                return this.makeNode(obj[2], aliases);
            }

            // Obj in not a let statement
            let args: Node[] = [];
            for (let i = 1; i < obj.length; ++i) {
                args.push(this.makeNode(obj[i], aliases));
            }

            // Add obj to this.nodes if not already there
            if (!this.nodes[obj[0]]) {
                this.nodes[obj[0]] = [];
            }

            let nodes = this.nodes[obj[0]];

            // Check if object with same name and arguments is already present
            for (let i = 0; i < nodes.length; ++i) {
                if (SMT.argsEqual(args, nodes[i].args)) {
                    return nodes[i];
                }
            }

            let node = new Node(obj[0], nodes.length, args);
            nodes.push(node);
            return node;
        }
    }
}