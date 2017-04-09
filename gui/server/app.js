    var express = require('express'); 
    var app = express(); 
    var bodyParser = require('body-parser');
    var sqlite = require('sqlite3').verbose();

    var file2 = "global.db";
    // var file2 = "prova.db";

    var fs = require('fs');
    require('ts-node/register')

    var tree = require("./src_ts/tree.ts");
    var solver = require("./src_ts/solver.ts");

    var dbTree;

    // Variable used to keep track how many rows of the db needs to be read
    var currenRow = 0;

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

            // Analyze db content
            currenRow = result.length;
            var tree = getDbResult(result);
            res.json(tree); //return result to the browser


        });
        db.close();
    });

    app.get('/getNext', function(req, res) { //get the content of the next row in the tree
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

            // Analyze db content
            if(currenRow != result.length){
                currenRow++;
            }
            getDbResult(result);


        });
        db.close();
    });

    app.get('/getPrevious', function(req, res) { //get the content of the next row in the tree
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

            // Analyze db content
            if(currenRow != 0){
                currenRow--;
            }
            getDbResult(result);


        });
        db.close();
    });

    app.get('/getEvents', function(req, res) {
        if(dbTree != undefined){
            res.json(dbTree.getEvents(currenRow)); //return result to the browser
        }
        else{
            res.json([]);
        }
    });

    app.get('/getSolvers', function(req, res) {
        if(dbTree != undefined){
            var db = new sqlite.Database(file2);
            var solvers = new Array();
            db.all("SELECT DISTINCT solver FROM SolvingHistory", function(err, rows) {
                rows.forEach(function (row) { // save each of the table as an object in array "result"
                    solvers.push(new solver.Solver(row.solver));

                });

                dbTree.solvers = solvers ;
                dbTree.assignSolvers(1,currenRow);
                res.json(dbTree.solvers)
            });
            db.close();
        }
        else{
            res.json([]);
        }
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

    function getDbResult(result) {
        dbTree = new tree.Tree();
        dbTree.createEvents(result);
        dbTree.arrangeTree(currenRow);

        //get the tree
        var treeView = dbTree.getTreeView();

        // write to the file to update treeView
        // if(treeView != undefined){
        //     fs.writeFile("../client/treeData.json", JSON.stringify(treeView, null, 5), function(err){
        //         if (err) {
        //             console.error(err);
        //             return;
        //         };
        //     });
        // }
        //
        // else {
        //     fs.writeFile("../client/treeData.json", null, function(err){
        //         if (err) {
        //             console.error(err);
        //             return;
        //         };
        //     });
        // }

        return treeView;


    }