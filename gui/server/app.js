const express = require('express');
const app = express();
const fileUpload = require('express-fileupload');
const bodyParser = require('body-parser');
const sqlite = require('sqlite3').verbose();
const pjson = require('./package.json');
const taskHandler = require('./taskHandler');
const sys = require('util');
const fs = require('fs');

let database;           // For past execution analysis
let isRealTime = false; // true if database is running on server
let port = 3000;        // Default port 3000

app.use(function (req, res, next) { //allow cross origin requests
    res.setHeader("Access-Control-Allow-Methods", "POST, PUT, OPTIONS, DELETE, GET");
    res.header("Access-Control-Allow-Origin", "http://localhost");
    res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    next();
});

// app.use(bodyParser.urlencoded({ extended : false,limit: '50mb' }));
app.use(bodyParser.json({limit: '50mb'}));
app.use(bodyParser.urlencoded({limit: '50mb', extended: true}));

app.use(express.static('../client'));
app.use(bodyParser.json());
app.use(fileUpload());

app.get('/get', function (req, res) { // get the content of the whole tree
    if (database === undefined) {
        console.log("No database was provided.");
        res.json([]); // return result to the browser
    }
    else {
        let db = new sqlite.Database(database);
        let result = [];
        db.all("SELECT * FROM SolvingHistory", function (err, rows) { // take everything from table "SolvingHistory"
            if (err) {
                res.json({error_code: 1, err_desc: err});
                return;
            }
            rows.forEach(function (row) { // save each of the table as an object in array "result"
                result.push({
                    id: row.id,
                    ts: row.ts,
                    name: row.name,
                    node: row.node,
                    event: row.event,
                    solver: row.solver,
                    data: row.data
                });
            });
            res.json(result); // return result to the browser
        });
        db.close();
    }
});

app.get('/get/:instance', function (req, res) { // Get content of a specific instance
    if (database === undefined) {
        console.log("No database was provided.");
        res.json([]); // return result to the browser
    }
    else {
        let instance = `'${req.params.instance}'`;
        let db = new sqlite.Database(database);
        let query = `SELECT * FROM SolvingHistory WHERE name=${instance}`;
        let result = [];
        db.all(query, function (err, rows) {
            if (err) {
                res.json({error_code: 1, err_desc: err});
                return;
            }
            rows.forEach(function (row) { // save each of the table as an object in array "result"
                result.push({
                    id: row.id,
                    ts: row.ts,
                    name: row.name,
                    node: row.node,
                    event: row.event,
                    solver: row.solver,
                    data: row.data
                });
            });
            res.json(result); // return result to the browser
        });
        db.close();
    }
});


app.get('/getInstances', function (req, res) {
    if (database === undefined) {
        console.log("No database was provided.");
        res.json([]); // return result to the browser
    }
    else {
        let db = new sqlite.Database(database);
        let result = [];
        db.all("SELECT DISTINCT name FROM SolvingHistory", function (err, rows) {
            if (err) {
                res.json({error_code: 1, err_desc: err});
                return;
            }
            rows.forEach(function (row) { // save each of the table as an object in array "result"
                result.push(row);
            });
            res.json(result); // return result to the browser
        });
        db.close();
    }
});

app.get('/getRealTime', function (req, res) {
    res.json(isRealTime);
});

app.post('/upload', function (req, res) {
    console.log('Uploading db file...');
    if (!req.files)
        return res.status(400).send('No files were uploaded.');

    // The name of the input field (i.e. "sampleFile") is used to retrieve the uploaded file
    let sampleFile = req.files.db;
    let uploadPath = __dirname + '/temp/' + sampleFile.name;

    if (!fs.existsSync(__dirname + '/temp')) {
        fs.mkdirSync(__dirname + '/temp');
    }

    // Use the mv() method to place the file somewhere on your server
    sampleFile.mv(uploadPath, function (err) {
        if (err) {
            return res.status(500).send(err);
        }
        // Set database
        database = './temp/' + sampleFile.name;
        console.log('File successfully uploaded.');
        res.redirect('back');
    });
});

app.get('/getServerData', function (req, res) {
    let response = taskHandler.getCurrent();
    res.json(response);
});

app.post('/changeTimeout', function (req, res) {
    // console.log(req.body.timeout);
    // console.log(req.body.type);
    if (req.body.type === "increase") {
        taskHandler.increaseTimeout(req.body.timeout);
    }
    else {
        taskHandler.decreaseTimeout(req.body.timeout);
    }
    // TODO: update timeout in index.html (when refreshing, and also try to prevent refresh)
    res.redirect('back');
});

app.post('/stop', function (req, res) {
    console.log("Stopping solving server execution..");
    //TODO: Check if it really stops
    taskHandler.stopSolving();
    res.redirect('back');
});


function deleteFile(file) {
    fs.unlink(file, function (err) {
        if (err) {
            console.error(err.toString());
        } else {
            console.warn(file + ' deleted');
        }
    });
}

process.stdin.resume();//so the program will not close instantly

// Delete all files in temp directory before killing the process
function exitHandler(options, err) {
    if (options.cleanup) {
        // console.log('Cleaning databases..');
        fs.readdir('./temp/', function (err, items) {
            for (let i = 0; i < items.length; i++) {
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
process.on('exit', exitHandler.bind(null, {cleanup: true}));

//catches ctrl+c event
process.on('SIGINT', exitHandler.bind(null, {cleanup: true}));

//catches uncaught exceptions
process.on('uncaughtException', exitHandler.bind(null, {exit: true}));


initialize();


function initialize() {
    if (process.argv[2] && (process.argv[2] === "--help" || process.argv[2] === "-h")) {
        showHelp();
        process.exit();
    }
    else if (process.argv[2] === "-v" || process.argv[2] === "--version") {
        console.log(pjson.version);
        process.exit();
    }
    else {
        for (let i = 2; i < process.argv.length - 1; i++) {
            switch (process.argv[i]) {
                // Port
                case '-p':
                case '--port':
                    let p = parseInt(process.argv[i + 1], 10);
                    if (p >= 0 && p < 65536) {
                        port = p;
                    }
                    else {
                        console.log("Bad or no port provided: 'port' argument must be >= 0 and < 65536");
                    }
                    break;

                // Database
                case '-d':
                case '--database':
                    database = process.argv[i + 1];
                    break;

                // Server
                case '-s':
                case'--server':
                    taskHandler.setPort(process.argv[i + 1]);
                    database = taskHandler.getDatabase();
                    isRealTime = true;
                    if (database === "") {
                        console.log("There is no database on the server provided. Closing SMT Viewer.")
                        process.exit();
                    }
                    break;
            }
        }

        // Quit if no database provided
        if (!database) {
            console.log('Error: no database wa provided');
            process.exit();
        }

        app.listen(port, function () {
            console.log('Server running on ' + port + '...');
        });


    }
}

function showHelp() {
    console.log("Usage: node app.js [-h] [-v] [-s SERVER] [-p PORT] [-d DATABASE]");
    console.log("");
    console.log("Options:");
    console.log("-h, --help                            show help message");
    console.log("-v, --version                         print SMT Viewer version");
    console.log("-s SERVER, --server SERVER            set server ip address");
    console.log("-p PORT, --port PORT                  set port");
    console.log("-d DATABASE, --database DATABASE      set database");
}