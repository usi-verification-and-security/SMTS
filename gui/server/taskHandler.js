
module.exports = {

    getInstance: function (exec) {
        exec("echo 'self.current.root.name if self.current else \"Empty\"'| python3.6 ../../server/client.py 3000", puts);
    },

    getDatabase:function (exec) {
        exec("echo 'self.config.db().execute(\"PRAGMA database_list\").fetchall()[0][2] if self.config.db() else \"Empty\"'|../../server/client.py 127.0.0.1:3000", putsDb);
        var res = db;
        db = "";
        return res;
    },

    executeAll: function (exec) {
        this.getInstance(exec);
        // this.whatever(exec);

        var res = response;
        response = [];
        return res;
    }

};
var response = [];
var db = "";
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
        db = stdout;
    }
}