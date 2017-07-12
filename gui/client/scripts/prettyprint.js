function prettyPrint(item) {
    if (item && typeof item === 'object') {
        let table = document.createElement('table');
        table.classList.add('smts-table');

        if (Object.keys(item).length > 0) {
            let header = table.createTHead().insertRow(0);
            header.appendChild(document.createElement('th')).innerText = 'Key';
            header.appendChild(document.createElement('th')).innerText = 'Value';
        }

        let body = table.appendChild(document.createElement('tbody'));
        for (let key in item) {
            let row = body.appendChild(document.createElement('tr'));
            row.insertCell(0).innerText = key;
            row.insertCell(1).appendChild(prettyPrint(item[key]));
        }

        return table;
    }

    let element = document.createElement('span');
    element.innerText = item;
    return element;
}