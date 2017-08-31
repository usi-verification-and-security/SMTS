////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// REQUIRES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Express
const express = require('express');
const app     = express();

// External libraries
const fileUpload = require('express-fileupload');
const bodyParser = require('body-parser');
const sqlite     = require('sqlite3').verbose();
const fs         = require('fs');
const io         = require('socket.io');
const tmp        = require('tmp');

// Custom libraries
const config      = require('./config.js');
const taskHandler = require('./taskHandler');


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND GLOBAL VARIABLES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// {Database}: The SQLite database
let database = null;

// Collection of utility functions
const tools = {

    // Set of statuses
    httpStatus: {
        0:   'Startup Error',
        200: 'OK',
        201: 'Created',
        400: 'Bad request',
        404: 'Not found',
        500: 'Internal Server Error'
    },

    // Send response to client as json
    // @param {Response} res: The response object.
    // @param {Number} status: HTTP status code of the response.
    // @param {Object} json: The json object to be sent to the client.
    sendJson: function(res, status, json) {
        res.status(status).json(json);
    },

    // Send response to client as an error
    // @param {Response} res: The response object.
    // @param {Number} status: HTTP status code of the error.
    // @param {Object} error: The json object representing the error. It is
    // made by the status code and a string description of the error.
    sendError: function(res, status, error, position) {
        let message = `${this.httpStatus[status]} ${position}: ${JSON.stringify(error)}`;
        res.status(status).json({status: status, error: message});
        console.error(message);
    },

    // Send response to client as an error and abort execution of appliction
    // @param {Response} res: The response object.
    // @param {Number} status: HTTP status code of the error.
    // @param {Object} error: The json object representing the error. It is
    // made by the status code and a string description of the error.
    sendFatal: function(res, status, error, position) {
        this.sendError(res, status, error, position);
        process.exit(0);
    },

    // Abort execution of appliction
    // @param {Number} status: HTTP status code of the error.
    // @param {String} error: The error message printed on stderr.
    fatal: function(status, error, position) {
        let message = `${this.httpStatus[status]} ${position}: ${error}`;
        console.error(message);
        process.exit(0);
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BASIC SETUP
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Allow cross origin requests
app.use(function(req, res, next) {
    res.setHeader("Access-Control-Allow-Methods", "POST, PUT, OPTIONS, DELETE, GET");
    res.header("Access-Control-Allow-Origin", "http://localhost");
    res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    next();
});

// app.use(bodyParser.urlencoded({ extended : false,limit: '50mb' }));
app.use(bodyParser.json({limit: '50mb'}));
app.use(bodyParser.urlencoded({limit: '50mb', extended: true}));

app.use(express.static('./www'));
app.use(bodyParser.json());
app.use(fileUpload());


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GET
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Get information about the execution
app.get('/info', function(req, res) {
    let info = {
        isLive:  taskHandler.isLive(),
        version: taskHandler.getVersion(),
        // TODO: add database name / server address
    };
    tools.sendJson(res, 200, info);
});

// Get information about the instance currently being solved
app.get('/solvingInfo', function(req, res) {
    let solving = taskHandler.getCurrent();
    tools.sendJson(res, 200, solving);
});

// Get instance CNF
// @params {String} type: Type of the CNF. Either `clauses` or `learnts`.
// @query {String} instanceName: The name of the instance.
// @query {String} value: If type is 'clauses', value is a node path. If type
// is 'learnts', value is a solver address.
app.get('/cnf/:type', function(req, res) {
    let pipename = '';
    let error = '';

    if (!req.query.instanceName) {
        error = 'No instance given';
    } else if (req.params.type === 'clauses') {
        if (req.query.value) {
            pipename = taskHandler.getCnfClauses(req.query.instanceName, req.query.value);
        } else {
            error = 'No node path given';
        }
    } else if (req.params.type === 'learnts') {
        if (req.query.value) {
            pipename = taskHandler.getCnfLearnts(req.query.instanceName, req.query.value);
        } else {
            error = 'No solver address given';
        }
    } else {
        error = 'No valid CNF type given (`clauses` or `learnts`)';
    }

    if (pipename) {
        tools.sendJson(res, 200, pipename);
    } else {
        tools.sendError(res, 500, error, 'GET /cnf');
    }
});

// Get all instances in database
app.get('/instances', function(req, res) {
    database.all("SELECT DISTINCT name FROM SolvingHistory", function(err, instances) {
        if (err) {
            tools.sendFatal(res, 500, err, 'GET /instances');
        } else if (!instances) {
            tools.sendFatal(res, 404, `Instances not found`, 'GET /instances');
        }
        tools.sendJson(res, 200, instances);
    });
});

// Get events associated with a particular instance
// @params {String} instance: The name of the instance.
// @query {Number} [optional] id: The id of the first event to be selected. If
// present only events starting from the given id will be sent, otherwise all.
app.get('/events/:instance', function(req, res) {
    let query = `SELECT * FROM SolvingHistory WHERE name='${req.params.instance}'`;
    if (req.query.id) {
        query += ` AND id >= ${req.query.id}`;
    }
    database.all(query, function(err, events) {
        if (err) {
            tools.sendFatal(res, 500, err, `GET /events/:instance`);
        } else if (!events) {
            tools.sendFatal(res, 404, `Events not found`, `GET /events/:instance`);
        }
        let eventsJson = [];
        events.forEach(function(event) {
            eventsJson.push({
                id:     event.id,
                ts:     event.ts,
                node:   JSON.parse(event.node),
                event:  event.event,
                solver: event.solver,
                data:   JSON.parse(event.data)
            });
        });
        tools.sendJson(res, 200, eventsJson);
    });
});


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FILES UPLOAD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Upload database to be used from the application
// @data {File[]} files: List of database file to use.
app.post('/upload/database', function(req, res) {
    if (!req.files) {
        tools.sendError(res, 400, 'No database file uploaded', 'POST /upload/database');
        return;
    }

    let file = req.files['smts-upload-database'];
    let dir = tmp.dirSync({unsafeCleanup: true});
    let path = `${dir.name}/${file.name}`;

    file.mv(path, function(err) {
        if (err) {
            tools.sendFatal(res, 500, 'Failed to save database file', 'POST /upload/database');
        }
        // Set database
        database = new sqlite.Database(path);
        res.redirect('back');
    });
});

// Upload instance to be solved by the application
// @data {File[]} files: List of instances files to use.
app.post('/upload/instance', function(req, res) {
    if (!req.files) {
        tools.sendError(res, 400, 'No instance file uploaded', 'POST /upload/instance');
        return;
    }

    let file = req.files['smts-upload-instance'];
    let dir = tmp.dirSync({unsafeCleanup: true});
    let path = `${dir.name}/${file.name}`;

    file.mv(path, function(err) {
        if (err) {
            tools.sendFatal(res, 500, err, 'POST /upload/instance');
        }
        // Start solving new instance
        taskHandler.newInstance(path);
        res.redirect('back');
    });
});


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TIMEOUT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Change solving timeout
// @data {Number} delta: The increase/decrease value with respect to the
// previous timeout.
app.post('/changeTimeout', function(req, res) {
    taskHandler.changeTimeout(req.body.delta);
    tools.sendJson(res, 201, {});
});

// Stop the currently solving execution
app.post('/stop', function(req, res) {
    taskHandler.stopSolving();
    tools.sendJson(res, 201, {});
});


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PARTITIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Partition a given solver
// @body {string} instanceName: Name of the instance.
// @body {string} nodePath: Path of the node to be partitioned.
app.post('/partition', function(req, res) {
    taskHandler.newPartition(req.body.instanceName, req.body.nodePath);
    tools.sendJson(res, 201, `Partition asked`);
});


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Initialize application and parse command line args
function initialize() {
    if (process.argv[2] && (process.argv[2] === "--help" || process.argv[2] === "-h")) {
        showHelp();
        process.exit();
    }
    else if (process.argv[2] === "-v" || process.argv[2] === "--version") {
        console.log(`SMT Viewer v${taskHandler.getVersion()}`);
        process.exit();
    }
    else {
        let databasePath;
        let portHttp = config.portHttp;

        for (let i = 2; i < process.argv.length - 1; i++) {
            switch (process.argv[i]) {
                // Port
                case '-p':
                case '--port':
                    portHttp = parseInt(process.argv[i + 1], 10);
                    if (portHttp < 0 || 65536 <= portHttp) {
                        tools.fatal(0, 'No valid port provided', 'initialize');
                        process.exit(0);
                    }
                    break;

                // Database
                case '-d':
                case '--database':
                    databasePath = process.argv[i + 1];
                    if (!databasePath) {
                        tools.fatal(0, 'No valid database provided', 'initialize');
                    }
                    break;

                // Server
                case '-s':
                case'--server':
                    taskHandler.setLive(true);
                    taskHandler.setPortServer(process.argv[i + 1]);
                    databasePath = taskHandler.getDatabase();
                    if (!databasePath) {
                        tools.fatal(0, 'No valid database on the server', 'initialize');
                    }
                    break;
            }
        }

        // Quit if no database provided
        if (!databasePath) {
            tools.fatal(0, 'No database provided', 'initialize');
        }

        // Open connection with database
        if (!fs.existsSync(databasePath)) {
            tools.fatal(0, 'Database file not found', 'initialize');
        }
        database = new sqlite.Database(databasePath);

        // Start application
        let server = app.listen(portHttp, function() {
            console.log(`GUI running on ${portHttp}...`);
        });

        // Setup socket server
        io.listen(server).on('connection', function(client) {
            client.on('get-cnf', function(pipename) {
                taskHandler.readPipeAndSend(pipename, client, 'get-cnf', true);
            });
        });
    }
}

// Catch unhandled exceptions
// @param {Exception} e: The unhandled exception.
function exceptionHandler(e) {
    // Uncaught error
    if (e instanceof taskHandler.ServerConnectionError) {
        // console.error(`Server Connection Error: ${err.stderr}`);
    } else if (e && e.stack) {
        console.error(e.stack);
    } else if (e) {
        console.error(e);
    }
    process.exit(0);
}

// Delete all files in temp directory before killing the process
function exitHandler() {
    // Close database connection
    if (database && database.open) {
        database.close();
    }
}

// Show help
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


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Catch uncaught exceptions
process.on('uncaughtException', exceptionHandler);

// Overload SIGINT event to make it go through exitHandler
process.on('SIGINT', () => process.exit(0));

// Catch `exit` event
process.on('exit', exitHandler);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

initialize();