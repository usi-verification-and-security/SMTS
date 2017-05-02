    var express = require('express');
    const app = express();
    const fileUpload = require('express-fileupload');
    var bodyParser = require('body-parser');
    var sqlite = require('sqlite3').verbose();

    var database = process.argv[2]; // use "global.db" for testing

    var fs = require('fs');

    app.use(function(req, res, next) { //allow cross origin requests
        res.setHeader("Access-Control-Allow-Methods", "POST, PUT, OPTIONS, DELETE, GET");
        res.header("Access-Control-Allow-Origin", "http://localhost");
        res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
        next();
    });

    app.use(express.static( '../client'));
    app.use(bodyParser.json());
    app.use(fileUpload());

    app.get('/get', function(req, res) { //get the content of the whole tree
        if(database ==  undefined){
            console.log("No database was provided.");
            res.json([]); //return result to the browser
        }
        else{
            var db = new sqlite.Database(database);
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
        }

    });

    app.get('/get/:instance', function(req, res) { // Get content of a specific instance
        if(database ==  undefined){
            console.log("No database was provided.");
            res.json([]); //return result to the browser
        }
        else{
            var inst = "'" + req.params.instance + "'";
            var db = new sqlite.Database(database);
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
        }

    });



    app.get('/getInstances', function(req, res) {
        if(database ==  undefined){
            console.log("No database was provided.");
            res.json([]); //return result to the browser
        }
        else{
            var db = new sqlite.Database(database);
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
        }

    });

    app.post('/upload', function(req, res) {
        console.log('Uploading db file...');
        if (!req.files)
            return res.status(400).send('No files were uploaded.');

        // The name of the input field (i.e. "sampleFile") is used to retrieve the uploaded file
        var sampleFile = req.files.db;
        var uploadPath = __dirname + '/temp/' + sampleFile.name;

        // Use the mv() method to place the file somewhere on your server
        sampleFile.mv(uploadPath, function(err) {
            if (err){
                return res.status(500).send(err);
            }
            // Set database
            database = './temp/' + sampleFile.name;
            console.log('File successfully uploaded.');
            res.redirect('back');
        });
    });

    function deleteFile (file) {
        fs.unlink(file, function (err) {
            if (err) {
                console.error(err.toString());
            } else {
                console.warn(file + ' deleted');
            }
        });
    };

    process.stdin.resume();//so the program will not close instantly

    // Delete all files in temp directory before killing the process
    function exitHandler(options, err) {
        if (options.cleanup){
            console.log('Cleaning databases..');
            fs.readdir('./temp/', function(err, items) {
                for (var i=0; i<items.length; i++) {
                    console.log(items[i]);
                    deleteFile('./temp/' + items[i]);
                }
                process.exit(0);
            });

        }
        if (err) console.log(err.stack);
        if (options.exit) process.exit();
    }

    //do something when app is closing
    process.on('exit', exitHandler.bind(null,{cleanup:true}));

    //catches ctrl+c event
    process.on('SIGINT', exitHandler.bind(null, {cleanup:true}));

    //catches uncaught exceptions
    process.on('uncaughtException', exitHandler.bind(null, {exit:true}));

    app.listen('3000', function(){
        console.log('Server running on 3000...');
    });
