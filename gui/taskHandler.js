'use strict';

const fs = require('fs');
let exec = require('child_process').execSync;
let config = require('./config.js');

class ServerConnectionError {

    constructor(e) {
        this.stderr = e.stderr;
        this.status = e.status;
    }
}

function run(command, args = '') {
    let value;
    try {
        let shellCmd = `echo 'json.dumps(${command})' | ${config.python} ${config.client} ${config.port} ${args}`;
        value = exec(shellCmd, {'encoding': 'utf8'});
        return eval(value || 'null');
    } catch (e) {
        console.error('----------ERROR----------');
        console.error(value);
        console.error('--------EXCEPTION--------');
        console.error(e);
        console.error('-----------END-----------');
        throw new ServerConnectionError(e);
    }
}

module.exports = {

    ServerConnectionError: ServerConnectionError,

    newInstance: function(filename) {
        return run(``, filename);
    },

    partition: function(nodePath, solverAddress) {
        return run(`self.partition("${nodePath.replace(/"/g, '\\\\"')}")`);
    },

    setPath: function(_path) {
        config.client = _path;
    },

    setPort: function(_port) {
        config.port = _port;
    },

    getVersion: function() {
        return parseInt(run(``, `--version`));
    },

    getDatabase: function() {
        return run(`self.config.db().execute("PRAGMA database_list").fetchall()[0][2] if self.config.db() else None`);
    },

    getInstance: function() {
        return run(`self.current.root.name if self.current else None`);
    },

    getCnfClauses: function(instanceName, nodePath) {
        let path = run(`self.get_cnf_clauses("${instanceName}", json.loads("${nodePath.replace(/"/g, '\\"')}"))`);
        return path ? this.pipeRead(`../${path}`) : '';
    },

    getCnfLearnts: function(instanceName, solverAddress) {
        let path = run(`self.get_cnf_learnts("${instanceName}", json.loads("${solverAddress.replace(/"/g, '\\"')}"))`);
        return path ? this.pipeRead(`../${path}`) : '';
    },

    stopSolving: function() {
        return run(`exec("if self.current: self.current.timeout=1")`);
    },

    changeTimeout: function(delta) {
        return run(`exec("if self.current: self.current.timeout+=${delta}")`);
    },

    getTimeout: function() {
        return run(`round(self.current.timeout, 2)`);
    },

    getElapsedTime: function() {
        return run(`round(time.time() - self.current.started, 2) if self.current else 0`);
    },

    getRemainingTime: function() {
        return run(`round(self.current.when_timeout, 2) if self.current else 0`);
    },

    getCurrent: function() {
        return {
            name: this.getInstance(),
            time: this.getElapsedTime(),
            left: this.getRemainingTime()
        };
    },

    pipeRead: function(pipename) {
        let data = fs.readFileSync(pipename);
        fs.unlinkSync(pipename);
        return data.toString();
    }
};
