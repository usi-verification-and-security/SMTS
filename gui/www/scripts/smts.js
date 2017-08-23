// Global object that contains all utilities
const smts = {};

smts.tools = {

    // Print formatted error message
    // @param {object} err: Error to be printed.
    error: function(err) {
        if (err.data && err.data.error) {
            console.log(`${err.data.status}: ${err.data.error}`);
        } else {
            console.log(err);
        }
    },

    // Display filename/error message for bootstrap file upload  forms
    // @param {Event} e: JS event.
    formBrowse: function(e) {
        let file = e.target;
        let form = file.parentNode.parentNode;
        let message = form.children[2];
        if (message) {
            let filename = file.value;
            if (!filename) {
                message.innerHTML = 'No file selected';
            } else {
                // Take only filename
                filename = filename.split('\\');
                filename = filename[filename.length - 1];
                message.innerHTML = filename;
            }
        }
    },

    // Display error message from HTTP request in form
    // @param {Event} e: JS event.
    formSubmit: function(e) {
        // TODO: show error message
    }
};