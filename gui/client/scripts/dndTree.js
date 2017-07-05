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

// Get JSON data
function getTreeJson(treeData, position) {
    let root = treeData;

    // Calculate total nodes, max label length
    let maxLabelLength = getMaxLabelLength(treeData);

    // Drag/drop
    let selectedNode = null;
    let draggingNode = null;

    // Misc.
    let i = 0;
    let duration = 0;

    // Size of the diagram
    let viewerWidth = document.getElementById("tree-container").offsetWidth;
    let viewerHeight = document.getElementById("tree-container").offsetHeight;

    // ???
    let tree = d3.layout.tree().size([viewerHeight, viewerWidth]);

    // Define a d3 diagonal projection for use by the node paths later on.
    let diagonal = d3.svg.diagonal().projection(function (d) {
        return [d.y, d.x];
    });

    // Define the zoomListener which calls the zoom function on the "zoom" event constrained within the scaleExtents
    let zoomListener = d3.behavior.zoom().scaleExtent([0.1, 3]).on("zoom", function () {
        svgGroup.attr("transform", `translate(${d3.event.translate}) scale(${d3.event.scale})`);
    });

    // Define the baseSvg, attaching a class for styling and the zoomListener
    d3.select("#tree-container").select("svg").remove(); // Delete previous treeView

    // Create svg element
    let baseSvg = d3.select("#tree-container")
        .append("svg")
        //for responsiveness
        .attr("viewBox", `0 0 ${viewerWidth} ${viewerHeight}`)
        .attr("preserveAspectRatio", "xMinYMin meet")
        // .attr("width", viewerWidth)
        // .attr("height", viewerHeight)
        .attr("class", "overlay")
        .call(zoomListener);

    // Append a group which holds all nodes and which the zoom Listener can act upon.
    let svgGroup = baseSvg.append("g");

    // Define the root
    root = treeData;
    if (root) {
        root.x0 = viewerHeight / 2;
        root.y0 = 0;

        // Layout the tree initially and center on the root node.
        update(root);

        if (!position) {
            centerNode(root);
        }
        else {
            d3.select('g')
                .transition()
                .duration(duration)
                .attr("transform", position);
            zoomListener.scale(scale);
            zoomListener.translate([x, y]);
        }
    }


    // FUNCTIONS

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

    // Update the temporary connector indicating dragging affiliation
    function updateTempConnector() {
        let data = [];
        if (draggingNode !== null && selectedNode !== null) {
            console.log('DRAGGING');
            // Flip the source coordinates since we did this for the existing connectors on the original tree
            data = [{
                source: {
                    x: selectedNode.y0,
                    y: selectedNode.x0
                },
                target: {
                    x: draggingNode.y0,
                    y: draggingNode.x0
                }
            }];
        }

        let link = svgGroup.selectAll(".templink").data(data);

        link.enter().append("path")
            .attr("class", "templink")
            .attr("d", d3.svg.diagonal())
            .attr('pointer-events', 'none');

        link.attr("d", d3.svg.diagonal());

        link.exit().remove();
    }

    // Function to center node when clicked/dropped so node doesn't get lost when collapsing/moving with large amount of children.
    function centerNode(source) {
        scale = zoomListener.scale();
        x = -source.y0;
        y = -source.x0;
        x = x * scale + viewerWidth / 2;
        y = y * scale + viewerHeight / 2;

        position = "translate(" + x + "," + y + ")scale(" + scale + ")";

        d3.select('g').transition()
            .duration(duration)
            .attr("transform", position);
        zoomListener.scale(scale);
        zoomListener.translate([x, y]);
    }

    function update(source) {
        // Compute the new height, function counts total children of root node and sets tree height accordingly.
        // This prevents the layout looking squashed when new nodes are made visible or looking sparse when nodes are removed
        // This makes the layout more consistent.
        let levelWidth = [1];
        let childCount = function (level, n) {

            if (n.children && n.children.length > 0) {
                if (levelWidth.length <= level + 1) levelWidth.push(0);

                levelWidth[level + 1] += n.children.length;
                n.children.forEach(function (d) {
                    childCount(level + 1, d);
                });
            }
        };
        childCount(0, root);
        let newHeight = d3.max(levelWidth) * 25; // 25 pixels per line
        tree = tree.size([newHeight, viewerWidth]);

        // Compute the new tree layout.
        let nodes = tree.nodes(root).reverse(),
            links = tree.links(nodes);

        // Set widths between levels based on maxLabelLength.
        nodes.forEach(function (d) {
            d.y = (d.depth * (maxLabelLength * 10)); //maxLabelLength * 10px
            // alternatively to keep a fixed scale one can set a fixed depth per level
            // Normalize for fixed-depth by commenting out below line
            // d.y = (d.depth * 500); //500px per level.
        });

        // Update the nodes…
        node = svgGroup.selectAll("g.node")
            .data(nodes, function (d) {
                return d.id || (d.id = ++i);
            });

        // Enter any new nodes at the parent's previous position.
        let nodeEnter = node.enter().append("g")
        // .call(dragListener)
            .attr("class", "node")
            .attr("transform", function (d) {
                return "translate(" + source.y0 + "," + source.x0 + ")";
            })
            .on('click', click);

        nodeEnter.append("circle")
            .attr("r", 0)
            .attr('class', function (d) {
                let c = "nodeCircle ";
                if (d.type == "OR") {
                    c += 'orNode';
                    return c;
                }
                else {
                    if (d.status == "sat") {
                        c += 'sat';
                        return c;
                    }
                    else if (d.status == "unsat") {
                        c += 'unsat';
                        return c;
                    }
                    else {
                        c += 'unknown';
                        return c;
                    }

                }
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

        // phantom node to give us mouseover in a radius around it
        // nodeEnter.append("circle")
        //     .attr('class', 'ghostCircle')
        //     .attr("r", 30)
        //     .attr("opacity", 1) // change this to zero to hide the target area
        //     .style("fill", "red")
        //     .attr('pointer-events', 'mouseover')
        //     .on("mouseover", function (node) {
        //         selectedNode = node;
        //         updateTempConnector();
        //     })
        //     .on("mouseout", function () {
        //         selectedNode = null;
        //         updateTempConnector();
        //     });

        // Update the text to reflect whether node has children or not.
        node.select('text')
            .attr("x", function (d) {
                return d.children || d._children ? -10 : 10;
            })
            .attr("text-anchor", function (d) {
                return d.children || d._children ? "end" : "start";
            })
            .text(function (d) {
                // return d.name;
                if (d.type == "AND") {
                    return d.solvers.length; //label is number of solvers working on the node
                }
                return null;
            });

        // Change the circle fill depending on whether it has children and is collapsed
        node.select("circle.nodeCircle")
            .attr("r", 4.5)
            .attr('class', function (d) {
                let c = "nodeCircle ";
                if (d.type == "OR") {
                    c += 'orNode';
                    return c;
                }
                else {
                    if (d.status == "sat") {
                        c += 'sat';
                        return c;
                    }
                    else if (d.status == "unsat") {
                        c += 'unsat';
                        return c;
                    }
                    else {
                        c += 'unknown';
                        return c;
                    }

                }
            });

        // Transition nodes to their new position.
        let nodeUpdate = node.transition()
            .duration(duration)
            .attr("transform", function (d) {
                return "translate(" + d.y + "," + d.x + ")";
            });

        // Fade the text in
        nodeUpdate.select("text")
            .style("fill-opacity", 1);

        // Transition exiting nodes to the parent's new position.
        let nodeExit = node.exit().transition()
            .duration(duration)
            .attr("transform", function (d) {
                return "translate(" + source.y + "," + source.x + ")";
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
                return diagonal({
                    source: o,
                    target: o
                });
            });

        // Transition links to their new position.
        link.transition()
            .duration(duration)
            .attr("d", diagonal);

        // Transition exiting nodes to the parent's new position.
        link.exit().transition()
            .duration(duration)
            .attr("d", function (d) {
                let o = {
                    x: source.x,
                    y: source.y
                };
                return diagonal({
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


        ///
        let object = {};

        let ppTable = prettyPrint(object);
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

function click(d) {
    showNodeData(d);
    highlightSolvers(d);

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
    let node = "[" + d.name.toString() + "]";
    let query = '.solver-container table tr[data-node="' + node + '"]';

    $('.solver-container table tr').removeClass("highlight");
    $(query).addClass("highlight");

}
