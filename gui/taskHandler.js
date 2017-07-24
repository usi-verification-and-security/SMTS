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
    try {
        let shellCmd = `echo 'json.dumps(${command})' | ${config.python} ${config.client} ${config.port} ${args}`;
        return JSON.parse(exec(shellCmd, {'encoding': 'utf8'}) || 'null');
    } catch (e) {
        throw new ServerConnectionError(e);
    }
}

module.exports = {

    ServerConnectionError: ServerConnectionError,

    newInstance: function(filename) {
        return run(``, filename);
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

    getCNF: function(instanceName) {
        for (let benchmarkPath of config.benchmarks_path) {
            let filePath = `${__dirname}/${benchmarkPath}/${instanceName}.smt2`;
            if (fs.existsSync(filePath)) {
                return exec(`../utils.py -s ${filePath}`, {'encoding': 'utf8'});
            }
        }
        return null;
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

    changeTimeout: function(delta) {
        return run(`exec("if self.current: self.current.timeout+=${delta}")`);
    },

    stopSolving: function() {
        return run(`exec("if self.current: self.current.timeout=1")`);
    }

};
