let exec = require('child_process').execSync;
let config = require('./config.js');

function run(command, args = '') {
    let cmd = `echo 'json.dumps(${command})' | ${config.python} ${config.client} ${config.port} ${args}`;
    return JSON.parse(exec(cmd, {'encoding': 'utf8'}) || 'null');
}

module.exports = {

    newInstance: function(filename) {
        return run(``, filename);
    },

    setPath: function (_path) {
        config.client = _path;
    },

    setPort: function (_port) {
        config.port = _port;
    },

    getVersion: function() {
        return parseInt(run(``, `--version`));
    },

    getDatabase: function () {
        return run(`self.config.db().execute("PRAGMA database_list").fetchall()[0][2] if self.config.db() else None`);
    },

    getInstance: function () {
        return run(`self.current.root.name if self.current else None`);
    },

    getTimeout: function() {
        return run(`round(self.current.timeout, 2)`);
    },

    getElapsedTime: function() {
        return run(`round(time.time() - self.current.started, 2) if self.current else 0`);
    },

    getRemainingTime: function () {
        return run(`round(self.current.when_timeout, 2) if self.current else 0`);
    },

    getCurrent: function () {
        return {
            name: this.getInstance(),
            time: this.getElapsedTime(),
            left: this.getRemainingTime()
        };
    },

    changeTimeout: function (delta) {
        return run(`exec("if self.current: self.current.timeout+=${delta}")`);
    },

    stopSolving: function () {
        return run(`exec("if self.current: self.current.timeout=1")`);
    }

};
