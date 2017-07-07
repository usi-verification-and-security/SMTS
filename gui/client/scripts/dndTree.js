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
/* CODE                                                                                                               */
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

// Remove the previous svg element
function clear() {
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

// Make the zoomListener which calls the zoom function on the "zoom" event constrained within the scaleExtents
function makeZoomListener(listener, target) {
    let zoomListener = d3.behavior.zoom().scaleExtent([0.1, 3]).on('zoom', function () {
        target.attr('transform', `translate(${d3.event.translate}) scale(${d3.event.scale})`);
    });
    listener.call(zoomListener);
    return zoomListener;
}

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

// Center node when clicked/dropped so node doesn't get lost when collapsing/moving with large amount of children.
function centerNode(node, width, height, duration, zoomListener) {
    let scale = zoomListener.scale();
    x = -node.y0 * scale + width / 2;
    y = -node.x0 * scale + height / 2;

    let position = `translate(${x},${y}) scale(${scale})`;

    d3.select('g')
        .attr('transform', position)
        .transition()
        .duration(duration);

    zoomListener.scale(scale);
    zoomListener.translate([x, y]);
}

// Get JSON data
function getTreeJson(root, selectedNodeNames, position) {

    // Calculate total nodes, max label length
    let maxLabelLength = getMaxLabelLength(root);

    // Misc.
    let i = 0;
    let duration = 0;

    // Size of the diagram
    let viewerWidth = document.getElementById("tree-container").offsetWidth;
    let viewerHeight = document.getElementById("tree-container").offsetHeight;

    // SVG element setup
    clear();
    let svg = makeSvg(viewerWidth, viewerHeight);
    let svgGroup = makeSvgGroup(svg);
    let zoomListener = makeZoomListener(svg, svgGroup);

    // Define the root
    if (root) {
        root.x0 = viewerHeight / 2;
        root.y0 = 0;

        // Layout the tree initially and center on the root node.
        update(root);

        if (!position) {
            centerNode(root, viewerWidth, viewerHeight, duration, zoomListener);
        }
        else {
            d3.select('g')
                .transition()
                .duration(duration)
                .attr("transform", position);
            //zoomListener.scale(scale);
            //zoomListener.translate([x, y]);
        }
    }

    function update(source) {

        let d3Tree = makeD3Tree(viewerWidth, getTreeHeight(root));
        let d3Diagonal = makeD3Diagonal();

        // Compute the new tree layout
        let nodes = d3Tree.nodes(root).reverse();
        let links = d3Tree.links(nodes);

        // Set widths between levels based on maxLabelLength
        nodes.forEach(function (node) {
            node.y = (node.depth * (maxLabelLength * 10));
        });

        // Update the nodes
        let svgNodes = svgGroup.selectAll("g.node")
            .data(nodes, function (node) {
                return node.id || (node.id = ++i);
            });

        // Enter any new nodes at the parent's previous position.
        let nodeEnter = svgNodes.enter().append("g")
            .classed('node', true)
            .classed('nodeAnd', node => node.type === 'AND')
            .classed('nodeOr', node => node.type === 'OR')
            .attr("transform", function () {
                return `translate(${source.y0},${source.x0})`;
            })
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
        nodeEnter.append("circle")
            .attr('r', '20')
            .attr("class", function (node) {
                let nodeNameStr = JSON.stringify(node.name);
                for (let selectedNodeName of selectedNodeNames) {
                    if (nodeNameStr === JSON.stringify(selectedNodeName)) {
                        return 'selected';
                    }
                }
                return 'hidden';
            });

        nodeEnter.append("text")
            .attr("x", function (d) {
                return d.children || d._children ? -10 : 10;
            })
            .attr("dy", ".35em")
            .attr('class', 'nodeText')
            .attr("text-anchor", function (d) {
                return d.children || d._children ? "end" : "start";
            })
            .text(function (d) {
                return d.name;
            })
            .style("fill-opacity", 0);

        // Update the text to reflect whether node has children or not
        svgNodes.select('text')
            .attr("x", function (d) {
                return d.children || d._children ? -10 : 10;
            })
            .attr("text-anchor", function (d) {
                return d.children || d._children ? "end" : "start";
            })
            .text(function (d) {
                // return d.name;
                if (d.type === "AND") {
                    return d.solvers.length; // Label is number of solvers working on the node
                }
                return null;
            });

        // Transition nodes to their new position.
        let nodeUpdate = svgNodes.transition()
            .duration(duration)
            .attr("transform", function (d) {
                return `translate(${d.y},${d.x})`;
            });

        // Fade the text in
        nodeUpdate.select("text")
            .style("fill-opacity", 1);

        // Transition exiting nodes to the parent's new position.
        let nodeExit = svgNodes.exit().transition()
            .duration(duration)
            .attr("transform", function () {
                return `translate(${source.y},${source.x})`;
            })
            .remove();

        nodeExit.select("circle")
            .attr("r", 0);

        nodeExit.select("text")
            .style("fill-opacity", 0);

        // Update the links…
        let link = svgGroup.selectAll("path.link")
            .data(links, function (d) {
                return d.target.id;
            });

        // Enter any new links at the parent's previous position.
        link.enter().insert("path", "g")
            .attr("class", "link")
            .attr("d", function (d) {
                let o = {
                    x: source.x0,
                    y: source.y0
                };
                return d3Diagonal({
                    source: o,
                    target: o
                });
            });

        // Transition links to their new position.
        link.transition()
            .duration(duration)
            .attr("d", d3Diagonal);

        // Transition exiting nodes to the parent's new position.
        link.exit().transition()
            .duration(duration)
            .attr("d", function (d) {
                let o = {
                    x: source.x,
                    y: source.y
                };
                return d3Diagonal({
                    source: o,
                    target: o
                });
            })
            .remove();

        // Stash the old positions for transition.
        nodes.forEach(function (d) {
            d.x0 = d.x;
            d.y0 = d.y;
        });

        let ppTable = prettyPrint({});
        document.getElementById('d6_1').innerHTML = "";
        let item = document.getElementById('d6_2');

        if (item.childNodes[0]) {
            item.replaceChild(ppTable, item.childNodes[0]); //Replace existing table
        }
        else {
            item.appendChild(ppTable);
        }
    }
}

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
