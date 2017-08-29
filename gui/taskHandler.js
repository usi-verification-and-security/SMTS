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
    if (config.isLive) {
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
    return null;
}

module.exports = {

    ServerConnectionError: ServerConnectionError,

    newInstance: function(filename) {
        return run(``, filename);
    },

    partition: function(instanceName, nodePath) {
        return run(`self.partition(self.current.root.child(${nodePath}), True) if self.current and self.current.root.name == "${instanceName}" else None`);
    },

    setPath: function(path) {
        config.client = path;
    },

    setPort: function(port) {
        config.port = port;
    },

    setLive: function(isLive) {
        config.isLive = isLive;
    },

    isLive: function() {
        return config.isLive;
    },

    getVersion: function() {
        try {
            return parseInt(exec(`${config.python} ${config.client} --version`));
        } catch (e) {
            return '!'
        }
    },

    getDatabase: function() {
        return run(`self.config.db().execute("PRAGMA database_list").fetchall()[0][2] if self.config.db() else None`);
    },

    getInstance: function() {
        return run(`self.current.root.name if self.current else None`);
    },

    getCnfClauses: function(instanceName, nodePath) {
        return run(`self.get_cnf_clauses("${instanceName}", ${nodePath})`);
    },

    getCnfLearnts: function(instanceName, solverAddress) {
        return run(`self.get_cnf_learnts("${instanceName}", ${solverAddress})`);
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

    readPipeAndSend: function(pipename, client, message) {
        fs.readFile(pipename, function(err, data) {
            client.emit(message, data.toString());
            fs.unlinkSync(pipename);
        });
    }
};
