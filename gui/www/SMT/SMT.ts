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
                    case 'declare-sort': this.makeType(obj);   break;
                    case 'declare-fun':  this.makeFn(obj);     break;
                    case 'assert':       this.makeAssert(obj); break;
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
        // @param {any} node: a node can be a string if it is a leaf, or a let
        // statement with the following structure
        // - obj[0] = 'let'
        // - obj[1] = letAlias[] // An array of alias objects
        // - obj[2] = childObj
        // or a function, with the following structure
        // - obj[0] = fnName
        // - obj[1]..obj[n] = fnArg
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
                let node = new Node(obj[0], i, args);
                if (node.argsEqual(nodes[i])) {
                    return nodes[i];
                }
            }

            let node = new Node(obj[0], nodes.length, args);
            nodes.push(node);
            return node;
        }

        // Process an assert object
        // @param {object} assert: The assert. It has the following structure
        // - assert[0] = 'assert'
        // - assert[1] = node
        makeAssert(assert: object) {
            this.root.args.push(this.makeNode(assert[1]));
        }

        // Make Type object and put it in this.types
        // @param {string} type: The name of the type.
        makeType(type: string) {
            this.types[type] = new Type(type);
        }

        // Make Fn object and put it in this.fns
        // @param [object] fn: The function. It has de following structure
        // - fn[0] = 'define-fun'
        // - fn[1] = fnName
        // - fn[2] = fnArgName[]
        makeFn(fn: object) {
            let types = [];
            for (let type of fn[2]) {
                types.push(this.types[type]);
            }
            this.fns[fn[1]] = new Fn(fn[1], types, this.types[fn[3]]);
        }

    }
}