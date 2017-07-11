function prettyPrint(item) {
    if (typeof item === 'object' || typeof item === 'array') {
        let table = document.createElement('table');
        table.classList.add('data-table');

        let i = 0;
        for (let key in item) {
            let row = table.insertRow(i++);
            row.insertCell(0).innerText = key;
            row.insertCell(1).appendChild(prettyPrint(item[key]));
        }

        return table;
    }

    let element = document.createElement('span');
    element.innerText = item;
    return element;
}