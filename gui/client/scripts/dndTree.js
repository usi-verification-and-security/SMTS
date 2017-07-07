/*Copyright (c) 2013-2016, Rob Schmuecker
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 * The name Rob Schmuecker may not be used to endorse or promote products
 derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL MICHAEL BOSTOCK BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/



/**********************************************************************************************************************/
/* CONSTANTS                                                                                                          */
/**********************************************************************************************************************/


const TRANSITION_DURATION = 0;



/**********************************************************************************************************************/
/* D3                                                                                                                 */
/**********************************************************************************************************************/


// Make a d3 tree
function makeD3Tree(width, height) {
    return d3.layout.tree().size([height, width]); // Width and height have swapped order
}


// Make a d3 diagonal projection for use by the node paths
function makeD3Diagonal() {
    return d3.svg.diagonal().projection(function (node) {
        return [node.y, node.x];
    });
}



/**********************************************************************************************************************/
/* SVG                                                                                                                */
/**********************************************************************************************************************/


// Remove the previous svg element
function clearSvg() {
    d3.select('#tree-container').select('svg').remove();
}


// Make an svg element
function makeSvg(width, height) {
    return d3.select('#tree-container')
        .append('svg')
        .attr('viewBox', `0 0 ${width} ${height}`)
        .attr('preserveAspectRatio', 'xMinYMin meet')
        .classed('overlay', true);
}


// Append a group which holds all nodes and on which the zoom Listener can act upon
function makeSvgGroup(svgBase) {
    return svgBase.append('g');
}



/**********************************************************************************************************************/
/* LISTENERS                                                                                                          */
/**********************************************************************************************************************/


// Make the zoomListener which calls the zoom function on the "zoom" event constrained within the scaleExtents
function makeZoomListener(listener, target) {
    let zoomListener = d3.behavior.zoom().scaleExtent([0.1, 3]).on('zoom', function () {
        target.attr('transform', `translate(${d3.event.translate}) scale(${d3.event.scale})`);
        scaleSelectedCircle(d3.event.scale);
    });
    listener.call(zoomListener);
    return zoomListener;
}



/**********************************************************************************************************************/
/* UTILS                                                                                                              */
/**********************************************************************************************************************/


// Get the max length of a label of all nodes in the given tree
function getMaxLabelLength(tree) {
    let maxLabelLength = 0;
    getMaxLabelLengthRec(tree);
    return maxLabelLength;

    function getMaxLabelLengthRec(node) {
        if (!node) {
            return;
        }
        maxLabelLength = Math.max(node.name.length, maxLabelLength);
        // Repete for children
        for (let child of node.children) {
            getMaxLabelLengthRec(child);
        }
    }
}


// Counts total children of tree and sets tree height accordingly
// This prevents the layout looking squashed when new nodes are made visible or looking sparse when nodes are removed,
// making the layout more consistent.
function getTreeHeight(tree) {
    let levelWidth = [1];
    getMaxLevelWidthRec(0, tree);
    return d3.max(levelWidth) * 25; // 25px per line

    function getMaxLevelWidthRec(level, node) {
        if (node.children && node.children.length > 0) {
            if (levelWidth.length <= level + 1) {
                levelWidth.push(0);
            }
            levelWidth[level + 1] += node.children.length;
            node.children.forEach(function (child) {
                getMaxLevelWidthRec(level + 1, child);
            });
        }
    }
}


// Check if two array have same content
function arrayEqual(a1, a2) {
    if (a1.length !== a2.length) {
        return false;
    }
    for (let i = 0; i < a1.length; ++i) {
        if (a1[i] !== a2[i]) {
            return false;
        }
    }
    return true;
}


// Check if nodeName is equal to at least one of the selectedNodeNames
function isSelectedNode(nodeName, selectedNodeNames) {
    for (let selectedNodeName of selectedNodeNames) {
        if (arrayEqual(nodeName, selectedNodeName)) {
            return true;
        }
    }
    return false;
}


// Return `[x, y]` from a transform string of the form 'translate(x, y) scale(z)', null if no translate
function getTranslate(position) {
    if (position && position.includes('translate')) {
        let values = position.match(/translate\(([^)]+)\)/)[1].split(',');
        return [parseFloat(values[0]), parseFloat(values[1])];
    }
    return null;
}


// Return `z` from a transform string of the form 'translate(x, y) scale(z)', null if no scale
function getScale(position) {
    if (position && position.includes('scale')) {
        return parseFloat(position.match(/scale\(([^)]+)\)/)[1]);
    }
    return null;
}



/**********************************************************************************************************************/
/* DOM TREE ELEMENTS                                                                                                  */
/**********************************************************************************************************************/


// Make nodes, put them in correct position and assign styles
function makeNodes(root, svgGroup, d3Nodes, selectedNodeNames) {
    let id = 0;
    let svgNodes = svgGroup.selectAll('g.node')
        .data(d3Nodes, node => node.id || (node.id = ++id));

    // Enter any new nodes at the parent's previous position
    let svgNodeGs = svgNodes.enter()
        .append('g')
        .classed('node', true)
        .classed('nodeAnd', node => node.type === 'AND') // Class needed as selector
        .classed('nodeOr', node => node.type === 'OR')   // Class needed as selector
        .attr('transform', `translate(${root.y0}, ${root.x0})`)
        .on('click', function (node) {
            showNodeData(node);
            highlightSolvers(node);
        });

    // Add rhombi to OR nodes
    svgGroup.selectAll('.nodeOr')
        .append('rect')
        .attr('width', '10')
        .attr('height', '10')
        .classed('nodeRect', true);

    // Add circles to AND nodes
    svgGroup.selectAll('.nodeAnd')
        .append('circle')
        .attr('r', '4.5')
        .classed('sat', node => node.status === 'sat')
        .classed('unsat', node => node.status === 'unsat')
        .classed('unknown', node => node.status === 'unknown')
        .classed('propagated', node => node.isStatusPropagated);

    // Make halo circle for selected node
    svgNodeGs.append('circle')
        .attr('r', '20')
        .attr('class', node => isSelectedNode(node.name, selectedNodeNames) ? 'selected' : 'hidden');

    // Make text
    svgNodeGs.append('text')
        .attr('x', node => node.children ? -10 : 10)
        .attr('dy', '.35em')
        .attr('text-anchor', node => node.children ? 'end' : 'start')
        .text(node => node.type === 'AND' ? node.solvers.length : null);

    // Transition nodes to their new position.
    svgNodes.transition()
        .duration(TRANSITION_DURATION)
        .attr('transform', node => `translate(${node.y},${node.x})`)
        .style('fill-opacity', 1);
}


// Make links between nodes, put them in correct position and assign styles
function makeLinks(root, svgGroup, d3Links) {
    let svgLinks = svgGroup.selectAll('path.link')
        .data(d3Links, link => link.target.id);

    let d3Diagonal = makeD3Diagonal();

    // Enter any new links at the parent's previous position.
    svgLinks.enter()
        .insert('path', 'g')
        .classed('link', true)
        .attr('d', () => d3Diagonal({source: {x: root.x0, y: root.y0}, target: {x: root.x0, y: root.y0}}));

    // Transition links to their new position.
    svgLinks.transition()
        .duration(TRANSITION_DURATION)
        .attr('d', d3Diagonal);

    // Transition exiting nodes to the parent's new position.
    svgLinks.exit()
        .transition()
        .duration(TRANSITION_DURATION)
        .attr('d', () => d3Diagonal({source: {x: root.x, y: root.y}, target: {x: root.x, y: root.y}}))
        .remove();
}



/**********************************************************************************************************************/
/* POSITIONING                                                                                                        */
/**********************************************************************************************************************/


// Scale selected node halo when zooming in or out
function scaleSelectedCircle(scale) {
    let circles = document.querySelectorAll('circle.selected');
    circles.forEach(circle => circle.setAttribute('r', (20 / scale).toString()));
}


// Move frame to x, y coordinates
function move(zoomListener, x, y, scale) {
    let position = `translate(${x}, ${y}) scale(${scale})`;

    d3.select('g')
        .attr('transform', position)
        .transition()
        .duration(TRANSITION_DURATION);

    scaleSelectedCircle(scale);

    zoomListener.scale(scale);
    zoomListener.translate([x, y]);
}


// Move frame centering x, y coordinates
function center(zoomListener, x, y, width, height, scale) {
    move(zoomListener, -y * scale + width / 2, -x * scale + height / 2, scale);
}


// Check if position is between bounds
function isInBounds(x, y, left, right, bottom, top) {
    let errX = (right - left) * 0.0125;
    let errY = (top - bottom) * 0.0125;
    return left + errX <= x && x < right - errX && bottom + errY <= y && y < top - errY;
}



/**********************************************************************************************************************/
/* MAIN                                                                                                               */
/**********************************************************************************************************************/


// Generate DOM tree
function generateDomTree(root, selectedNodeNames, positionFrame) {

    if (!root) {
        return;
    }

    // Size of the diagram
    let viewerWidth = document.getElementById("tree-container").offsetWidth;
    let viewerHeight = document.getElementById("tree-container").offsetHeight;

    // SVG element setup
    clearSvg();
    let svg = makeSvg(viewerWidth, viewerHeight);
    let svgGroup = makeSvgGroup(svg);
    let zoomListener = makeZoomListener(svg, svgGroup);

    // Define the root
    root.x0 = viewerHeight / 2;
    root.y0 = 0;

    // Calculate max label length
    // This has to be computed before reversing the root
    let maxLabelLength = getMaxLabelLength(root);

    // Compute the new tree layout
    let d3Tree = makeD3Tree(viewerWidth, getTreeHeight(root));
    let d3Nodes = d3Tree.nodes(root).reverse();
    let d3Links = d3Tree.links(d3Nodes);

    // Set widths between levels based on maxLabelLength
    d3Nodes.forEach(node => node.y = (node.depth * (maxLabelLength * 10)));

    // Generate DOM tree
    makeNodes(root, svgGroup, d3Nodes, selectedNodeNames);
    makeLinks(root, svgGroup, d3Links);

    // Stash the old positions for transition
    d3Nodes.forEach(function (node) {
        node.x0 = node.x;
        node.y0 = node.y;
    });

    // ???
    let ppTable = prettyPrint({});
    document.getElementById('d6_1').innerHTML = '';
    let item = document.getElementById('d6_2');

    // ???
    if (item.childNodes[0]) {
        item.replaceChild(ppTable, item.childNodes[0]); // Replace existing table
    }
    else {
        item.appendChild(ppTable);
    }

    // Set position
    if (positionFrame) {
        let positionSelected = document.querySelector('circle.selected').parentNode.getAttribute('transform');
        let translateSelected = getTranslate(positionSelected);
        let translateFrame = getTranslate(positionFrame);
        let scale = getScale(positionFrame) || zoomListener.scale();
        let x = translateSelected[0] * scale + translateFrame[0];
        let y = translateSelected[1] * scale + translateFrame[1];

        if (isInBounds(x, y, 0, viewerWidth, 0, viewerHeight)) {
            move(zoomListener, translateFrame[0], translateFrame[1], scale);
        }
        else {
            let node = root.getNode(selectedNodeNames[0]);
            center(zoomListener, node.x0, node.y0, viewerWidth, viewerHeight, getScale(positionFrame) || zoomListener.scale());
        }
    }
    else {
        let node = root.getNode(selectedNodeNames[0]);
        center(zoomListener, node.x0, node.y0, viewerWidth, viewerHeight, zoomListener.scale());
    }
}



/**********************************************************************************************************************/
/* OTHER                                                                                                              */
/**********************************************************************************************************************/


// This function shows node data in data view
function showNodeData(d) {
    let object = {};
    object.name = d.name.toString(); // transform to string or it will show and array
    object.type = d.type;
    object.solvers = d.solvers;
    object.status = d.status;

    let ppTable = prettyPrint(object);
    document.getElementById('d6_1').innerHTML = "Node".bold();
    let item = document.getElementById('d6_2');

    if (item.childNodes[0]) {
        item.replaceChild(ppTable, item.childNodes[0]); //Replace existing table
    }
    else {
        item.appendChild(ppTable);
    }
}


// This function highlights in solver view the solvers working on the clicked node
function highlightSolvers(d) {
    let node = JSON.stringify(d.name);
    let query = `.solver-container table tr[data-node="${node}"]`;

    $('.solver-container table tr').removeClass("highlight");
    $(query).addClass("highlight");
}
