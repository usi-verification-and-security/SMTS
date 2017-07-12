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


// Node spacing
const LINE_HEIGHT = 25; // Space between to nodes (in pixels)
const LINK_LENGTH = 10; // Distance between two nodes (in pixels)

// Nodes
const NODE_AND_RADIUS      = 4.5; // Radius of circle (in pixels)
const NODE_OR_SIDE         = 10;  // Width of rhombus side (in pixels)
const NODE_SELECTED_RADIUS = 20;  // Radius of selected node circle (in pixels)
const TEXT_OFFSET          = 10;  // Text offset with respect to the center of the circle/rhombus node (in pixels)

// Frame
const BOUNDS_ERROR_FACTOR = 0.0125; // Factor to compute the margin of the visible frame

// Scale
const SCALE_MIN = 0.1; // Minimum scale factor
const SCALE_MAX = 3.0; // Maximum scale factor

// Frame movement
const TRANSITION_DURATION = 0; // Duration of frame transition to another node (in milliseconds)



/**********************************************************************************************************************/
/* MAIN                                                                                                               */
/**********************************************************************************************************************/


// Generate DOM tree
function generateDomTree(root, selectedNodeNames, positionFrame) {

    if (!root) {
        return;
    }

    // Size of the diagram
    let viewerWidth = document.getElementById("smts-tree-container").offsetWidth;
    let viewerHeight = document.getElementById("smts-tree-container").offsetHeight;

    // SVG element setup
    clearSvg();
    let svg = makeSvg(viewerWidth, viewerHeight);
    let svgGroup = makeSvgGroup(svg);
    let zoomListener = makeZoomListener(svg, svgGroup);

    // Define the root
    root.x0 = viewerHeight / 2;
    root.y0 = 0;

    // Calculate max label length
    // This has to be computed before reversing the root.
    let maxLabelLength = root.getMaxLabelLength();

    // Compute the new tree layout
    // Using `getMaxLevelWidth` prevents the layout looking squashed when new nodes are made visible or looking sparse
    // when nodes are removed, making the layout more consistent.
    let d3Tree = makeD3Tree(viewerWidth, getMaxLevelWidth(root) * LINE_HEIGHT);
    let d3Nodes = d3Tree.nodes(root).reverse();
    let d3Links = d3Tree.links(d3Nodes);

    // Set widths between levels based on maxLabelLength
    d3Nodes.forEach(node => node.y = (node.depth * (maxLabelLength * LINK_LENGTH)));

    // Generate DOM tree
    makeNodes(root, svgGroup, d3Nodes, selectedNodeNames);
    makeLinks(root, svgGroup, d3Links);

    // Stash the old positions for transition
    d3Nodes.forEach(function (node) {
        node.x0 = node.x;
        node.y0 = node.y;
    });

    // Update data table with selected node data
    let selectedNode = root.getNode(selectedNodeNames[0]);
    showNodeData(selectedNode);
    highlightSolvers(selectedNode);

    // Move view in right position
    // Center selected node if not in visible frame, otherwise restore the view as it was before.
    // Function is in callback because it has to wait until the translate positions are updated inside the `g` elements.
    setTimeout(function() {
        if (positionFrame) {
            let positionSelected = document.querySelector('circle.smts-selected').parentNode.getAttribute('transform');
            let translateSelected = getTranslate(positionSelected);
            let translateFrame = getTranslate(positionFrame);
            let scale = getScale(positionFrame) || zoomListener.scale();
            let x = translateSelected[0] * scale + translateFrame[0];
            let y = translateSelected[1] * scale + translateFrame[1];

            if (isInBounds(x, y, 0, viewerWidth, 0, viewerHeight)) {
                move(zoomListener, translateFrame[0], translateFrame[1], scale);
            }
            else {
                let scale = getScale(positionFrame) || zoomListener.scale();
                center(zoomListener, selectedNode.x0, selectedNode.y0, viewerWidth, viewerHeight, scale);
            }
        }
        else {
            center(zoomListener, selectedNode.x0, selectedNode.y0, viewerWidth, viewerHeight, zoomListener.scale());
        }
    }, 0);
}



/**********************************************************************************************************************/
/* SVG                                                                                                                */
/**********************************************************************************************************************/


// Remove the previous svg element
function clearSvg() {
    d3.select('#smts-tree-container').select('svg').remove();
}


// Make an svg element
function makeSvg(width, height) {
    return d3.select('#smts-tree-container')
        .append('svg')
        .attr('viewBox', `0 0 ${width} ${height}`)
        .attr('preserveAspectRatio', 'xMinYMin meet')
        .classed('smts-overlay', true);
}


// Append a group which holds all nodes and on which the zoom Listener can act upon
function makeSvgGroup(svgBase) {
    return svgBase.append('g');
}



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
/* ZOOM LISTENER                                                                                                      */
/**********************************************************************************************************************/


// Make the zoomListener which calls the zoom function on the "zoom" event constrained within the scaleExtents
function makeZoomListener(listener, target) {
    let zoomListener = d3.behavior.zoom().scaleExtent([SCALE_MIN, SCALE_MAX]).on('zoom', function () {
        target.attr('transform', `translate(${d3.event.translate}) scale(${d3.event.scale})`);
        scaleSelectedCircle(d3.event.scale);
    });
    listener.call(zoomListener);
    return zoomListener;
}



/**********************************************************************************************************************/
/* DOM TREE ELEMENTS                                                                                                  */
/**********************************************************************************************************************/


// Make nodes, put them in correct position and assign styles
function makeNodes(root, svgGroup, d3Nodes, selectedNodeNames) {
    let id = 0;
    let svgNodes = svgGroup.selectAll('g.smts-node')
        .data(d3Nodes, node => node.id || (node.id = ++id));

    // Enter any new nodes at the parent's previous position
    let svgNodeGs = svgNodes.enter()
        .append('g')
        .classed('smts-node', true)
        .classed('smts-nodeAnd', node => node.type === 'AND') // Class needed as selector
        .classed('smts-nodeOr', node => node.type === 'OR')   // Class needed as selector
        .attr('transform', `translate(${root.y0}, ${root.x0})`)
        .on('click', function (node) {
            showNodeData(node);
            highlightSolvers(node);
        });

    // Add rhombi to OR nodes
    svgGroup.selectAll('.smts-nodeOr')
        .append('rect')
        .attr('width', NODE_OR_SIDE)
        .attr('height', NODE_OR_SIDE)
        .classed('smts-nodeRect', true);

    // Add circles to AND nodes
    svgGroup.selectAll('.smts-nodeAnd')
        .append('circle')
        .attr('r', NODE_AND_RADIUS)
        .classed('smts-sat', node => node.status === 'sat')
        .classed('smts-unsat', node => node.status === 'unsat')
        .classed('smts-unknown', node => node.status === 'unknown')
        .classed('smts-propagated', node => node.isStatusPropagated);

    // Make halo circle for selected node
    svgNodeGs.append('circle')
        .attr('r', NODE_SELECTED_RADIUS)
        .attr('class', node => isSelectedNode(node.name, selectedNodeNames) ? 'smts-selected' : 'smts-hidden');

    // Make text
    svgNodeGs.append('text')
        .attr('x', node => node.children ? -TEXT_OFFSET : TEXT_OFFSET)
        .attr('dy', '.35em')
        .attr('text-anchor', node => node.children ? 'end' : 'start')
        .text(node => node.type === 'AND' ? node.solvers.length : null);

    // Transition nodes to their new position.
    svgNodes.transition()
        .duration(TRANSITION_DURATION)
        .attr('transform', node => `translate(${node.y},${node.x})`);
}


// Make links between nodes, put them in correct position and assign styles
function makeLinks(root, svgGroup, d3Links) {
    let svgLinks = svgGroup.selectAll('path.smts-link')
        .data(d3Links, link => link.target.id);

    let d3Diagonal = makeD3Diagonal();

    // Enter any new links at the parent's previous position.
    svgLinks.enter()
        .insert('path', 'g')
        .classed('smts-link', true)
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
    let circles = document.querySelectorAll('circle.smts-selected');
    circles.forEach(circle => circle.setAttribute('r', (NODE_SELECTED_RADIUS / scale).toString()));
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
    let errX = (right - left) * BOUNDS_ERROR_FACTOR;
    let errY = (top - bottom) * BOUNDS_ERROR_FACTOR;
    return left + errX <= x && x < right - errX && bottom + errY <= y && y < top - errY;
}



/**********************************************************************************************************************/
/* UTILS                                                                                                              */
/**********************************************************************************************************************/


// Count total number of nodes in each level of depth, and return the greatest
function getMaxLevelWidth(tree) {
    let levelWidths = [1];
    getMaxLevelWidthRec(tree, 0);
    return d3.max(levelWidths);

    function getMaxLevelWidthRec(node, level) {
        if (node.children && node.children.length > 0) {
            if (levelWidths.length <= level + 1) {
                levelWidths.push(0);
            }
            levelWidths[level + 1] += node.children.length;
            node.children.forEach(child => getMaxLevelWidthRec(child, level + 1));
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
/* DATA VIEW AND HIGHLIGHTS                                                                                           */
/**********************************************************************************************************************/


// This function shows node data in data view
function showNodeData(node) {
    let ppNode = {};

    if (node) {
        ppNode.name = JSON.stringify(node.name); // transform to string or it will show and array
        ppNode.type = node.type;
        ppNode.solvers = node.solvers;
        ppNode.status = node.status;
        ppNode.depth = node.getDepth();
        if (node.children) {
            ppNode.depths = {};
            for (let child of node.children) {
                ppNode.depths[JSON.stringify(child.name)] = child.getDepth();
            }
        }
    }

    let ppTable = prettyPrint(ppNode);

    document.getElementById('smts-data-container-title').innerHTML = 'NODE'.bold();
    let item = document.getElementById('smts-data-container-content');
    item.innerText = '';
    item.appendChild(ppTable);
}


// This function highlights in solver view the solvers working on the clicked node
function highlightSolvers(node) {
    let solvers = document.querySelectorAll('#smts-solver-container table tr');
    solvers.forEach(solver => solver.classList.remove('smts-highlight'));

    let query = `#smts-solver-container table tr[data-node="${JSON.stringify(node.name)}"]`;
    let selectedSolvers = document.querySelectorAll(query);
    selectedSolvers.forEach(selectedSolver => selectedSolver.classList.add('smts-highlight'));
}
