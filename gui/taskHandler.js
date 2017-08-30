'use strict';


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// REQUIRES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const fs     = require('fs');
const execAsync = require('child_process').exec;
const exec   = require('child_process').execSync;
const config = require('./config.js');


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RUN FUNCTION
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Handle errors in `run` function
class ServerConnectionError {
    constructor(e) {
        this.stderr = e.stderr;
        this.status = e.status;
    }
}

// Execute a command with a list of arguments
// It allows to call server functions through the shell, and send back to the
// client their returned values. It only runs the command if it is live mode.
// Examples of commands run by `run` through the shell:
//
//     run("round(self.current.timeout, 2)") => echo 'json.dumps(round(self.current.timeout, 2))' | python3 ../client.py 3000
//
//     run("", "instance.smt2")              => echo 'json.dumps()' | python3 ../client.py 3000 instance.smt2
//
// @param {string} command: Python command compatible with the SMTS server
// application.
// @param {string} args [default='']: Optional arguments to be passed to the
// shell command.
function run(command, args = '') {
    if (config.isLive) {
        let value;
        try {
            let shellCmd = `echo 'json.dumps(${command})' | ${config.python} ${config.client} ${config.portServer} ${args}`;
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


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EXPORT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Object containing intermediary functions used by `app.js`
module.exports = {

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CLASSES AND GLOBAL VARIABLES
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Redefinition of `ServerConnectionError` for error handling in `app.js`
    ServerConnectionError: ServerConnectionError,


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // GETTERS / SETTERS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Set path of client application
    // Used in case `app.js` wants to override the default client path.
    // @param {string} path: Path of the client application.
    // E.g.: '../client.py'.
    setClient: function(client) {
        config.client = client;
    },

    // Set port of the GUI application
    // Used in case `app.js` wants to override the default GUI server port.
    // @param {number} portHttp: Port number of the GUI server application.
    setPortHttp: function(portHttp) {
        config.portHttp = portHttp;
    },

    // Set port of the server python application
    // Used in case `app.js` wants to override the default server port.
    // @param {number} portServer: Port number of the server application.
    setPortServer: function(portServer) {
        config.portServer = portServer;
    },

    // Set whether the GUI application is running in live or database mode
    // Taskhandler needs to know if it is live or not, so that it will execute
    // or not the `run()` function depending on the mode.
    // `app.js` should always call this function.
    // @param {boolean} isLive: `true` if GUI is live mode, `false` otherwise.
    setLive: function(isLive) {
        config.isLive = isLive;
    },

    // Check if GUI is running on live mode or not
    // @return {boolean}: `true` if GUI is live mode, `false` otherwise.
    isLive: function() {
        return config.isLive;
    },

    // Get version number of the application
    // @return {number/string}: If it executes correctly, a number representing
    // the version of the program is returned. If an error occurred, '!' is
    // returned instead.
    getVersion: function() {
        try {
            return parseInt(exec(`${config.python} ${config.client} --version`));
        } catch (e) {
            return '!'
        }
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // LIVE MODE GETTERS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Get CNF clauses of given node
    // @param {string} instanceName: Name of the instance.
    // @param {string} nodePath: Path of the node, as string. Once the string
    // is put into the string template, python will interpret it as object[],
    // not a string (since no quotes surround it).
    // Example of node path: '[ 1, 2, 3, 4 ]'
    // @return {string}: Clauses CNF of the node, or empty string if not
    // available.
    getCnfClauses: function(instanceName, nodePath) {
        return run(`self.get_cnf_clauses("${instanceName}", ${nodePath})`);
    },

    // Get CNF learnts of given solver
    // @param {string} instanceName: Name of the instance.
    // @param {string} solverAddress: Address of the solver, as string.
    // Once the string is put into the string template, python will interpret
    // it as object[], not a string (since no quotes surround it).
    // The learnt CNF is available only if the server is currently solving the
    // given instance.
    // Example of solver address: '[ "127.0.0.1", 1234 ]'
    // @return {string}: Learnts CNF of the solver, or empty string if not
    // available.
    getCnfLearnts: function(instanceName, solverAddress) {
        return run(`self.get_cnf_learnts("${instanceName}", ${solverAddress})`);
    },

    // Get name of currently used database by the server
    // @return {string}: Name of the database.
    getDatabase: function() {
        return run(`self.config.db().execute("PRAGMA database_list").fetchall()[0][2] if self.config.db() else None`);
    },

    // Get name of current instance
    // @return {string}: Name of the instance.
    getInstance: function() {
        return run(`self.current.root.name if self.current else None`);
    },

    // Get elapsed time since the current instance has started solving
    // @return {number}: The elapsed time in seconds since the instance has
    // started, or `0` if no instance is currently being solved.
    getElapsedTime: function() {
        return run(`round(time.time() - self.current.started, 2) if self.current else 0`);
    },

    // Get remaining time until timeout goes off and solving is interrupted
    // @return {number}: The remaining time in seconds until timeout, or `0` if
    // no instance is currently being solved.
    getRemainingTime: function() {
        return run(`round(self.current.when_timeout, 2) if self.current else 0`);
    },

    // Get current instance name, elapsed time and remaining time until timeout
    // The function wraps all information concerning the execution of the
    // current instance into a single object.
    // @return {object}: An object with the following form:
    //     {
    //         name: string // Name of the instance
    //         time: number // Elapsed time since instance has started solving
    //         left: number // Remaining time until instance timeout
    //      }
    getCurrent: function() {
        return {
            name: this.getInstance(),
            time: this.getElapsedTime(),
            left: this.getRemainingTime()
        };
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // LIVE MODE SETTERS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Start new instance solving on server, corresponding to file name
    // @param {string} filename: Name of the file containing the new instance
    // to be solved.
    // @return {null}: Null. The return tsatement is here for consistency with
    // other functions.
    newInstance: function(filename) {
        return run(``, filename);
    },

    // Partition AND node of running instance into more nodes.
    // @param {string} instanceName: Name of the instance running the node.
    // @param {string} nodePath: Path of the node to be partitioned, as string.
    // Once the string is put into the string template, python will interpret
    // it as object[], not a string (since no quotes surround it).
    // @return {null}: Null. The return tsatement is here for consistency with
    // other functions.
    newPartition: function(instanceName, nodePath) {
        return run(`self.partition(self.current.root.child(${nodePath}), True) if self.current and self.current.root.name == "${instanceName}" else None`);
    },

    // Stop solving the currently solving instance
    stopSolving: function() {
        return run(`exec("if self.current: self.current.timeout=1")`);
    },

    // Change the current timeout of the currently running instance
    // @param {number} delta: Variation, in seconds, of the timeout.
    // E.g.: delta = -100, timeout = 400 => new timeout = 300
    changeTimeout: function(delta) {
        return run(`exec("if self.current: self.current.timeout+=${delta}")`);
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // WEBSOCKET FUNCTIONS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Read content of pipe and send it to client via websocket
    // Pipes are used for asynchronous communication between the python server
    // and the GUI application. The content of the pipe is sent as a string to
    // the client.
    // E.g.: CNF request from client to server.
    // @param {string} pipename: Name of the pipe to be read.
    // @param {io.Socket} client: Client socket to which the content of the
    // file is sent.
    // @param {string} event: Name of the event on which the client is waiting
    // a response. E.g.: 'get-cnf'.
    // @param {boolean} removePipe: If `true`, the pipe is removed after
    // the read is done.
    readPipeAndSend: function(pipename, client, event, removePipe) {
        // Cat content of pipename to get CNF
        let cmd = `cat ${pipename}`;
        let maxSize = 1024 * 1024 * 10; // 10MB
        execAsync(cmd, {maxBuffer: maxSize}, (err, data) => {
            client.emit(event, data);
            if (removePipe) fs.unlink(pipename, () => {});
        });
    }
};
