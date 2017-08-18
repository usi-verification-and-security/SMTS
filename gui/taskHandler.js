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
        let r = exec(shellCmd, {'encoding': 'utf8'});
        return JSON.parse(r || 'null');
    } catch (e) {
        console.log(e);
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

    // getCNF: function(instanceName) {
    //     for (let benchmarkPath of config.benchmarks_path) {
    //         let filePath = `${__dirname}/${benchmarkPath}/${instanceName}.smt2`;
    //         if (fs.existsSync(filePath)) {
    //             return exec(`../utils.py -s ${filePath}`, {'encoding': 'utf8'});
    //         }
    //     }
    //     return null;
    // },

    getCnf: function(instanceName) {
        let path = run(`self.get_cnf("${instanceName}")`);
        return path ? this.pipeRead(`../${path}`, '}') : '';
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

    pipeRead: function(pipename, endChar) {
        // Mode 'r' (read-only) causes problems, 'r+' is required even without
        // having to write in the pipe
        let fd = fs.openSync(pipename, 'r+');

        let buffer = new Buffer(1024);
        let data = '', lastChar = '', size = 0;

        while (lastChar !== endChar) {
            size = fs.readSync(fd, buffer, 0, buffer.length, null);
            let readData = buffer.slice(0, size).toString();
            lastChar = size > 0 ? readData[size - 1] : '';
            data += readData;
        }

        // Close and remove pipe
        fs.closeSync(fd);
        fs.unlinkSync(pipename);

        return data;
    }
};
