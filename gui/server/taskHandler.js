
var exec = require('child_process').exec;

module.exports = {

    getInstance: function () {
        exec("echo 'self.current.root.name if self.current else \"Empty\"'| python3.6 ../../server/client.py 3000", puts);
    },

    getRemainingTime: function () {
        exec("echo 'self.current.when_timeout if self.current else \"Empty\"'| python3.6 ../../server/client.py 3000", puts);
    },

    getDatabase:function () {
        exec("echo 'self.config.db().execute(\"PRAGMA database_list\").fetchall()[0][2] if self.config.db() else \"Empty\"'|../../server/client.py 127.0.0.1:3000", putsDb);
        var result = res;
        res = "";
        return result;
    },

    executeAll: function () {

        this.getInstance();
        this.getRemainingTime();

        var res = response;
        response = [];
        return res;
    },

    increaseTimeout: function (timeout) {
        console.log("Increasing solver timeout by " + timeout);
        exec("echo 'exec('self.current.timeout+='"+ timeout + "')' if self.current != 'None' else \"Nothing to reset\" '|../../server/client.py 127.0.0.1:3000", putsDb);

    },

    decreaseTimeout: function (timeout) {
        console.log("Decreasing solver timeout by " + timeout);
        exec("echo 'exec('self.current.timeout-='"+ timeout + "')' if self.current != 'None' else \"Nothing to reset\" '|../../server/client.py 127.0.0.1:3000", putsDb);

    },

    stopSolving: function () {
        exec("echo 'exec('self.current.timeout=1')' if self.current != 'None' else \"Nothing to reset\" '|../../server/client.py 127.0.0.1:3000", putsDb);
    }

};

var response = [];
var res = "";
/*
    response[0] = instance
    response[1] = remaining time
 */


function puts(error, stdout, stderr) {
    if(stdout){
        response.push(stdout);
    }

    if(stderr){
        response.push("Empty");
    }
}

function putsDb(error, stdout, stderr) {
    if(stdout){
        res = stdout;
    }
}