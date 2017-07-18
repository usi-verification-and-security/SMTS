let exec = require('child_process').execSync;
let config = require('./config.js');

function run(command, args = '') {
    return exec(`echo '${command}' | ${config.python} ${config.client} ${config.port} ${args}`, {'encoding': 'utf8'}).trim();
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
        return run(`self.config.db().execute("PRAGMA database_list").fetchall()[0][2] if self.config.db() else "Empty"`);
    },

    getInstance: function () {
        return run(`self.current.root.name if self.current else "Empty"`);
    },

    getTimeout: function() {
        return run(`self.current.timeout`);
    },

    getElapsedTime: function() {
        return run(`time.time() - self.current.when_started`);
    },

    getRemainingTime: function () {
        return run(`self.current.when_timeout if self.current else "Empty"`);
    },

    getCurrent: function () {
        return [this.getInstance(), this.getRemainingTime()];
    },

    changeTimeout: function (delta) {
        return run(`exec("self.current.timeout+=${delta}") if self.current != None else "Nothing to reset"`);
    },

    stopSolving: function () {
        return run(`exec("self.current.timeout=1") if self.current != None else "Nothing to reset"`);
    }

};
