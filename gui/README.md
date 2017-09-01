# SMT Viewer
SMT Viewer is an application for SMT instances visualization. Here follows a basic guide on how to run the application.

## Installation
From the SMTS main directory, first go to the GUI application directory
```
cd gui
```
To install the SMT Viewer, run
```
npm install
```
When you first run `npm install`, the installation might take some time.
Ignore the warning given by Bower, saying that it is deprecated.

## Usage
SMT Viewer can be execute in two different modes: **live** and **database**.

### Database mode
Database mode allows to visualize already existing databases. For each instance present in the database, it is possible to see all events occurring.
To run in database mode, do
```
npm start -- -d DB_NAME
```
Where `DB_NAME` is the path to the database, relative to the current directory.

### Live mode
Live mode allows to solve instances live. While the instance is being solved, it is possible to see how it evolves.
To run in live mode, do
```
npm start -- -s SERVER_PORT
```
Where `SERVER_PORT` is the port used by the server.

### Options
Both in live and database mode, it is possible to specify on which port SMT Viewer application is running on. To do so, run the application with the `-p` flag
```
npm start -- -s SERVER_PORT -p PORT
```
Where `PORT` is the port number. The default port is `8080`.

## Examples
A few example on how to run SMT Viewer
* `npm start -- -s 3000`
* `npm start -- -s 4000 -p 3000`
* `npm start -- -d demo.db`
* `npm start -- -d ../databases/demo.db -p 8000`

## Help
To display the help menu in the shell, run
```
npm start -- -h
```
To see the current version of the application, run
```
npm start -- -v
```
