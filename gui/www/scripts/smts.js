// Global object that contains all utilities
const smts = {};

smts.tools = {

    error: function(err) {
        console.log(`Error: ${JSON.stringify(err)}`);
    },

    formBrowse: function(e) {
        let file = e.target;
        let form = file.parentNode.parentNode;
        let message = form.children[2];//form.getElementsByClassName('smts-message')[0];
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

    formSubmit: function(e) {
        // TODO: show error message
    }
};