// Settings used in `TaskHandler.js`

const config = {
    client: '../server/client.py',         // Path to the server python file
    port:   3000,                   // Server port
    portHttp: 8000,
    python: 'python3',               // Python command-line
    benchmarks_path: ["benchmarks"]
};

module.exports = config;
