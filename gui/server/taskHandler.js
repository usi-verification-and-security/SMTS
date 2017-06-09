
module.exports = {

    getInstance: function (exec) {
        exec("echo 'self.current.root.name if self.current else \"Empty\"'| python3.6 ../../server/client.py 3000", puts);
    },

    getDatabase:function (exec) {
        exec("echo 'self.config.db().execute(\"PRAGMA database_list\").fetchall()[0][2] if self.config.db() else \"Empty\"'|../../server/client.py 127.0.0.1:3000", putsDb);
        var result = res;
        res = "";
        return result;
    },

    executeAll: function (exec) {
        this.getInstance(exec);
        // this.whatever(exec);

        var res = response;
        response = [];
        return res;
    },

    setTimeout: function (exec, timeout) {
        // self.config.solving_timeout
        exec("echo 'self.config.solving_timeout=" + timeout + " if self.config.solving_timeout else \"Nothing to reset\" '|../../server/client.py 127.0.0.1:3000", putsDb);

    },

    stopSolving: function (exec) {
        exec("echo 'self.current.stop() if self.current else \"Nothing to stop\" '|../../server/client.py 127.0.0.1:3000", putsDb);
        var result = res;
        res = "";
        return result;
    }

};

var response = [];
var res = "";
/*
    response[0] = instance
 */

function puts(error, stdout, stderr) {
    // console.log(stdout);
    if(stdout){
        response.push(stdout);
    }

    // if(stderr){
        // console.log("Empty");
        // response.push("Empty");
    // }
    // return stdout;
    // console.log(stderr);
}

function putsDb(error, stdout, stderr) {
    if(stdout){
        res = stdout;
    }
}