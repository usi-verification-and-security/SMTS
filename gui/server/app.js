    var express = require('express'); 
    var app = express(); 
    var bodyParser = require('body-parser');
    var sqlite = require('sqlite3').verbose();

    var file2 = "global.db";
    // var file2 = "prova.db";

    var fs = require('fs');

    app.use(function(req, res, next) { //allow cross origin requests
        res.setHeader("Access-Control-Allow-Methods", "POST, PUT, OPTIONS, DELETE, GET");
        res.header("Access-Control-Allow-Origin", "http://localhost");
        res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
        next();
    });

    app.use(express.static( '../client'));
    app.use(bodyParser.json());  

    app.get('/get', function(req, res) { //get the content of the whole tree
        var db = new sqlite.Database(file2);
        var result = [];
        db.all("SELECT * FROM SolvingHistory", function(err, rows) { //  take everything from table "SolvingHistory"
            if(err){
                res.json({error_code:1,err_desc:err});
                return;
            }
            rows.forEach(function (row) { // save each of the table as an object in array "result"
                result.push({id: row.id, ts: row.ts, name: row.name, node: row.node, event: row.event, solver: row.solver, data: row.data});
            });

            res.json(result); //return result to the browser


        });
        db.close();
    });

    app.get('/get/:instance', function(req, res) { // Get content of a specific instance
        var inst = "'" + req.params.instance + "'";
        var db = new sqlite.Database(file2);
        var query = "SELECT * FROM SolvingHistory WHERE name=" + inst;
        var result = [];
        db.all(query, function(err, rows) {
            if(err){
                res.json({error_code:1,err_desc:err});
                return;
            }
            rows.forEach(function (row) { // save each of the table as an object in array "result"
                result.push({id: row.id, ts: row.ts, name: row.name, node: row.node, event: row.event, solver: row.solver, data: row.data});
            });

            res.json(result); //return result to the browser


        });
        db.close();
    });



    app.get('/getInstances', function(req, res) {
        var db = new sqlite.Database(file2);
        var result = [];
        db.all("SELECT DISTINCT name FROM SolvingHistory", function(err, rows) {
            if(err){
                res.json({error_code:1,err_desc:err});
                return;
            }
            rows.forEach(function (row) { // save each of the table as an object in array "result"
                result.push(row);
            });
            // console.log(result);
            res.json(result); //return result to the browser

        });
        db.close();
    });

    app.listen('3000', function(){
        console.log('Server running on 3000...');
    });
