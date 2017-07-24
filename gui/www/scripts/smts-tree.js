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

// Set of functions that manipulate the DOM object 'smts-tree'
smts.tree = {

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CONSTANTS AND GLOBAL VARIABLES
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Node spacing
    LINE_HEIGHT: 25, // Space between to nodes (in pixels)
    LINK_LENGTH: 10, // Distance between two nodes (in pixels)

    // Nodes
    NODE_AND_RADIUS: 4.5,        // Radius of circle (in pixels)
    NODE_OR_SIDE: 10,            // Width of rhombus side (in pixels)
    NODE_SELECTED_RADIUS: 20,    // Radius of selected node circle (in pixels)
    NODE_BALANCENESS_RADIUS: 30, // Radius of height ratio circle (in pixels)
    TEXT_OFFSET: 10,             // Text offset with respect to the center of the circle/rhombus node (in pixels)

    // Frame
    BOUNDS_ERROR_FACTOR: 0.0125, // Factor to compute the margin of the visible frame

    // Scale
    SCALE_MIN: 0.1, // Minimum scale factor
    SCALE_MAX: 3.0, // Maximum scale factor
    g_scale: 1,

    // Frame movement
    TRANSITION_DURATION: 0, // Duration of frame transition to another node (in milliseconds)


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MAKE TREE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Generate DOM tree
    make: function(tree, positionFrame) {

        let root = tree.root;

        if (!root) {
            return;
        }

        // Size of the diagram
        let viewerWidth = document.getElementById("smts-svg-tab-content").offsetWidth;
        let viewerHeight = document.getElementById("smts-svg-tab-content").offsetHeight;

        // SVG element setup
        this.clearSvg();
        let svg = this.makeSvg(viewerWidth, viewerHeight);
        let svgGroup = this.makeSvgGroup(svg);
        let zoomListener = this.makeZoomListener(svg, svgGroup);

        // Define the root
        root.x0 = viewerHeight / 2;
        root.y0 = 0;

        // Calculate max label length
        // This has to be computed before reversing the root.
        let maxLabelLength = root.getMaxLabelLength();

        // Compute the new tree layout
        // Using `getMaxLevelWidth` prevents the layout looking squashed when new nodes are made visible or looking sparse
        // when nodes are removed, making the layout more consistent.
        let d3Tree = this.makeD3Tree(viewerWidth, this.getMaxLevelWidth(root) * this.LINE_HEIGHT);
        let d3Nodes = d3Tree.nodes(root).reverse();
        let d3Links = d3Tree.links(d3Nodes);

        // Set widths between levels based on maxLabelLength
        d3Nodes.forEach(node => node.y = (node.depth * (maxLabelLength * this.LINK_LENGTH)));

        // Generate DOM tree
        this.makeNodes(tree, svgGroup, d3Nodes);
        this.makeLinks(tree, svgGroup, d3Links);

        // Stash the old positions for transition
        d3Nodes.forEach(function(node) {
            node.x0 = node.x;
            node.y0 = node.y;
        });

        // Update data table with selected node data
        let selectedNode = tree.selectedNodes[0];
        smts.tables.data.update(selectedNode, 'node');

        // Move view in correct position
        window.setTimeout(function() {
            smts.tree.centerTree(positionFrame, viewerWidth, viewerHeight, selectedNode, zoomListener);
            d3.select('#smts-tree').classed('smts-hidden', false);
        }, 0);
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ACCESSORS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Get the position of the tree
    // @return {String}: The position in the form 'translate(174,150) scale(1)'
    // or `null` if the tree is not present.
    getPosition: function() {
        let tree = document.getElementById('smts-tree');
        return tree ? tree.getAttribute('transform') : null;
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ZOOM LISTENER
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Make the zoomListener which calls the zoom function on the "zoom" event constrained within the scaleExtents
    makeZoomListener: function(listener, target) {
        let zoomListener = d3.behavior.zoom().scaleExtent([this.SCALE_MIN, this.SCALE_MAX]).on('zoom', function() {
            target.attr('transform', `translate(${d3.event.translate}) scale(${d3.event.scale})`);
            smts.tree.setScale(d3.event.scale);
        });
        listener.call(zoomListener);
        return zoomListener;
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SVG CONTAINER
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Remove the previous svg element
    clearSvg: function() {
        d3.select('#smts-tree-container').select('svg').remove();
    },

    // Make an svg element
    makeSvg: function(width, height) {
        return d3.select('#smts-tree-container')
            .append('svg')
            .attr('viewBox', `0 0 ${width} ${height}`)
            .attr('preserveAspectRatio', 'xMinYMin meet')
            .classed('smts-overlay', true);
    },

    // Append a group which holds all nodes and on which the zoom Listener can act upon
    makeSvgGroup: function(svgBase) {
        return svgBase.append('g')
            .attr('id', 'smts-tree')
            // Hide tree until it is positioned correctly, to avoid annoying transition
            .classed('smts-hidden', true);
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// D3 ELEMENTS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Make a d3 tree
    makeD3Tree: function(width, height) {
        // Width and height have swapped order
        return d3.layout.tree().size([height, width]);
    },

    // Make a d3 diagonal projection for use by the node paths
    makeD3Diagonal: function() {
        return d3.svg.diagonal().projection(node => [node.y, node.x]);
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NODES
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Make nodes, put them in correct position and assign styles
    makeNodes: function(tree, svgGroup, d3Nodes) {
        let id = 0;
        let svgNodes = svgGroup.selectAll('g.smts-node')
            .data(d3Nodes, node => node.id || (node.id = ++id));

        // Enter any new nodes at the parent's previous position
        let svgNodeGs = svgNodes.enter()
            .append('g')
            .classed('smts-node', true)
            // Classes needed as selectors
            .classed('smts-nodeAnd', node => node.type === 'AND')
            .classed('smts-nodeOr', node => node.type === 'OR')
            // Check if node matches any element of tree.selectedNodes
            .classed('smts-nodeSelected', node => node.equalAny(tree.selectedNodes))
            .attr('transform', `translate(${tree.root.y0}, ${tree.root.x0})`)
            .on('click', function(node) {
                smts.tables.data.update(node, 'node');
                tree.setSelectedNodes([node]);
                smts.tree.updateSelectedNode(this, tree); // `this` is the DOM element
            });

        // Add rhombi to OR nodes
        svgGroup.selectAll('.smts-nodeOr')
            .append('rect')
            .attr('width', this.NODE_OR_SIDE)
            .attr('height', this.NODE_OR_SIDE)
            .classed('smts-nodeRect', true);

        // Make balanceness circle for OR nodes
        let balancenessOption = document.getElementById('smts-option-balanceness');
        svgGroup.selectAll('.smts-nodeOr')
            .append('circle')
            .attr('r', this.NODE_BALANCENESS_RADIUS)
            .classed('smts-balanceness', true)
            // Hide if balancenessOption doesn't exists or if the checkbox is unchecked
            .classed('smts-hidden', !balancenessOption || !balancenessOption.checked)
            .style('fill', function(node) {
                let balanceness = node.getBalanceness();
                let red = Math.round(255 * (1 - balanceness));
                let green = Math.round(255 * balanceness);
                return `rgb(${red}, ${green}, 0)`;
            });

        // Add circles to AND nodes
        svgGroup.selectAll('.smts-nodeAnd')
            .append('circle')
            .attr('r', this.NODE_AND_RADIUS)
            .classed('smts-sat', node => node.status === 'sat')
            .classed('smts-unsat', node => node.status === 'unsat')
            .classed('smts-unknown', node => node.status === 'unknown')
            .classed('smts-propagated', node => node.isStatusPropagated);

        // Make halo circle for selected node
        svgGroup.selectAll('.smts-nodeSelected')
            .append('circle')
            .attr('r', this.NODE_SELECTED_RADIUS)
            .classed('smts-selected', true);

        // Make text
        svgNodeGs.append('text')
            .attr('x', node => node.children ? -this.TEXT_OFFSET : this.TEXT_OFFSET)
            .attr('dy', '.35em')
            .attr('text-anchor', node => node.children ? 'end' : 'start')
            .text(node => node.type === 'AND' ? node.solvers.length : null);

        // Transition nodes to their new position.
        svgNodes.transition()
            .duration(this.TRANSITION_DURATION)
            .attr('transform', node => `translate(${node.y},${node.x})`);
    },

    // Update selected node
    updateSelectedNode: function(node, tree) {
        // Remove previous selections
        d3.selectAll('.smts-nodeSelected')
            .classed('smts-nodeSelected', false)
            .selectAll('.smts-selected')
            .remove();

        // Select new node
        d3.select(node)
            .classed('smts-nodeSelected', true)
            .append('circle')
            .attr('r', this.NODE_SELECTED_RADIUS / this.g_scale)
            .classed('smts-selected', true);

        // Update events if 'Selected' tab is selected
        smts.tables.events.update(tree.selectedNodes);
        smts.tables.solvers.update(tree.selectedNodes);
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LINKS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Make links between nodes, put them in correct position and assign styles
    makeLinks: function(tree, svgGroup, d3Links) {

        let root = tree.root;

        let svgLinks = svgGroup.selectAll('path.smts-link')
            .data(d3Links, link => link.target.id);

        let d3Diagonal = this.makeD3Diagonal();

        // Enter any new links at the parent's previous position.
        svgLinks.enter()
            .insert('path', 'g')
            .classed('smts-link', true)
            .attr('d', () => d3Diagonal({source: {x: root.x0, y: root.y0}, target: {x: root.x0, y: root.y0}}));

        // Transition links to their new position.
        svgLinks.transition()
            .duration(this.TRANSITION_DURATION)
            .attr('d', d3Diagonal);

        // Transition exiting nodes to the parent's new position.
        svgLinks.exit()
            .transition()
            .duration(this.TRANSITION_DURATION)
            .attr('d', () => d3Diagonal({source: {x: root.x, y: root.y}, target: {x: root.x, y: root.y}}))
            .remove();
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// POSITIONING
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Set global scale value and call all scaling functions
    setScale: function(scale) {
        this.g_scale = scale;
        this.scaleSelectedCircles();
    },

    // Scale selected node halo when zooming in or out
    scaleSelectedCircles: function() {
        let circles = document.querySelectorAll('circle.smts-selected');
        circles.forEach(circle => circle.setAttribute('r', (this.NODE_SELECTED_RADIUS / this.g_scale).toString()));
    },

    // Move frame to x, y coordinates
    move: function(zoomListener, x, y, scale) {
        let position = `translate(${x}, ${y}) scale(${scale})`;

        d3.select('g')
            .attr('transform', position)
            .transition()
            .duration(this.TRANSITION_DURATION);

        this.setScale(scale);

        zoomListener.scale(scale);
        zoomListener.translate([x, y]);
    },

    // Move frame centering x, y coordinates
    center: function(zoomListener, x, y, width, height, scale) {
        this.move(zoomListener, -y * scale + width / 2, -x * scale + height / 2, scale);
    },

    // Check if position is between bounds
    isInBounds: function(x, y, left, right, bottom, top) {
        let errX = (right - left) * this.BOUNDS_ERROR_FACTOR;
        let errY = (top - bottom) * this.BOUNDS_ERROR_FACTOR;
        return left + errX <= x && x < right - errX && bottom + errY <= y && y < top - errY;
    },

    // Center selected node if not in visible frame, otherwise restore the view as it was before
    centerTree: function(positionFrame, viewerWidth, viewerHeight, node, zoomListener) {
        if (positionFrame) {
            let positionSelected = document.querySelector('circle.smts-selected').parentNode.getAttribute('transform');
            let translateSelected = this.getTranslate(positionSelected);
            let translateFrame = this.getTranslate(positionFrame);
            let scale = this.getScale(positionFrame) || zoomListener.scale();
            let x = translateSelected[0] * scale + translateFrame[0];
            let y = translateSelected[1] * scale + translateFrame[1];

            if (this.isInBounds(x, y, 0, viewerWidth, 0, viewerHeight)) {
                this.move(zoomListener, translateFrame[0], translateFrame[1], scale);
            }
            else {
                let scale = this.getScale(positionFrame) || zoomListener.scale();
                this.center(zoomListener, node.x0, node.y0, viewerWidth, viewerHeight, scale);
            }
        }
        else {
            this.center(zoomListener, node.x0, node.y0, viewerWidth, viewerHeight, zoomListener.scale());
        }
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OTHER
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Count total number of nodes in each level of depth, and return the greatest
    getMaxLevelWidth: function(tree) {
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
    },

    // Return `[x, y]` from a transform string of the form 'translate(x, y) scale(z)', null if no translate
    getTranslate: function(position) {
        if (position && position.includes('translate')) {
            let values = position.match(/translate\(([^)]+)\)/)[1].split(',');
            return [parseFloat(values[0]), parseFloat(values[1])];
        }
        return null;
    },

    // Return `z` from a transform string of the form 'translate(x, y) scale(z)', null if no scale
    getScale: function(position) {
        if (position && position.includes('scale')) {
            return parseFloat(position.match(/scale\(([^)]+)\)/)[1]);
        }
        return null;
    }

};