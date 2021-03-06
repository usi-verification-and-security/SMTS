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

    // Tree
    tree: null,

    // Node spacing
    LINE_HEIGHT: 35, // Vertical space between to nodes (in pixels)
    LINK_LENGTH: 50, // Horizontal space between two nodes (in pixels)

    // Nodes
    NODE_AND_RADIUS: 4.5,        // Radius of circle (in pixels)
    NODE_SELECTED_RADIUS: 20,    // Radius of selected node circle (in pixels)
    NODE_BALANCENESS_RADIUS: 30, // Radius of height ratio circle (in pixels)
    TEXT_OFFSET: 10,             // Text offset with respect to the center of the circle/rhombus node (in pixels)

    // Frame
    BOUNDS_ERROR_FACTOR: 0.0125, // Factor to compute the margin of the visible frame

    // Scale
    SCALE_MIN: 0.1, // Minimum scale factor
    SCALE_MAX: 3.0, // Maximum scale factor
    scale: 1,       // Current scale factor

    // Frame movement
    TRANSITION_DURATION: 0, // Duration of frame transition to another node (in milliseconds)


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TREE MANIPULATION
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Build DOM tree, based on `this.tree`
    build: function() {

        let root = this.tree.root;

        if (!root) {
            return;
        }

        // Size of the diagram
        let treeContainer = document.getElementById("smts-tree-container");
        let width = treeContainer.offsetWidth;
        let height = treeContainer.offsetHeight;

        // SVG element setup
        this.clearSvg();
        let svg = this.makeSvg(width, height);
        let svgGroup = this.makeSvgGroup(svg);
        let zoomListener = this.makeZoomListener(svg, svgGroup);

        // Define the root
        root.x0 = height / 2;
        root.y0 = 0;

        // Compute the new tree layout
        // Using `getMaxLevelWidth` prevents the layout looking squashed when
        // new nodes are made visible or looking sparse when nodes are removed,
        // making the layout more consistent.
        let d3Tree = this.makeD3Tree(width, this.getMaxLevelWidth(root) * this.LINE_HEIGHT);
        let d3Nodes = d3Tree.nodes(root).reverse();
        let d3Links = d3Tree.links(d3Nodes);

        // Set widths between levels based on maxLabelLength
        d3Nodes.forEach(node => node.y = (node.depth * this.LINK_LENGTH));

        // Generate DOM tree
        this.populateNodes(svgGroup, d3Nodes);
        this.populateLinks(svgGroup, d3Links);

        // Stash the old positions for transition
        d3Nodes.forEach(function(node) {
            node.x0 = node.x;
            node.y0 = node.y;
        });

        // Update data table with selected node data
        let selectedNode = this.tree.selectedNodes[0];
        smts.data.update(selectedNode, 'node');

        // Move view in correct position
        // For some reason, the function must be in a callback
        window.setTimeout(() => {
            this.centerTree(width, height, selectedNode, zoomListener);
            d3.select('#smts-tree').classed('smts-hidden', false);
        }, 0);
    },

    // Update tree
    // @param {object[]} events: List of events needed to build a new tree.
    update: function(events) {
        this.tree = new TreeManager.Tree(events);
    },

    // Toggle balanceness halos visibility surrounding OR nodes
    toggleBalanceness: function() {
        let balancenessOption = document.getElementById('smts-option-balanceness');
        let balancenessHalos = document.querySelectorAll('.smts-balanceness');
        if (balancenessOption.checked) {
            for (let balancenessHalo of balancenessHalos) {
                balancenessHalo.classList.remove('smts-hidden');
            }
        } else {
            for (let balancenessHalo of balancenessHalos) {
                balancenessHalo.classList.add('smts-hidden');
            }
        }
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ACCESSORS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Get the position of the tree
    // @return {string}: The position in the form 'translate(174,150) scale(1)'
    // or `null` if the tree is not present.
    getPosition: function() {
        let tree = document.getElementById('smts-tree');
        return tree ? tree.getAttribute('transform') : null;
    },

    // Get list of selected nodes
    // @return {Node[]}: The list of selected nodes.
    getSelectedNodes: function() {
        return this.tree.selectedNodes;
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ZOOM LISTENER
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Make the zoomListener
    // The `zoomListener` calls the zoom function on the 'zoom' event,
    // constrained within the `scaleExtents`.
    // @param {DOMElement} listener: The element which listens to the 'zoom'
    // event.
    // @param {DOMElement} target: The element affected by the zoom (the zoomed
    // element).
    // @return {d3.ZoomBehavior}: The zoom behavior used by the d3 library.
    makeZoomListener: function(listener, target) {
        // Update scale on zoom
        let zoomListener = d3.behavior.zoom().scaleExtent([this.SCALE_MIN, this.SCALE_MAX]).on('zoom', () => {
            target.attr('transform', `translate(${d3.event.translate}) scale(${d3.event.scale})`);
            this.setScale(d3.event.scale);
        });
        // Call listener and remove zoom on double click
        listener.call(zoomListener).on("dblclick.zoom", null);
        return zoomListener;
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SVG CONTAINER
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Remove the previous tree SVG element
    clearSvg: function() {
        d3.select('#smts-tree-container').select('svg').remove();
    },

    // Make a SVG element, which will be the base for the new tree
    // @param {number} width: Width of the container.
    // @param {number} heigh: Height of the container.
    // @return {DOMElement}: The SVG element.
    makeSvg: function(width, height) {
        return d3.select('#smts-tree-container')
            .append('svg')
            .attr('viewBox', `0 0 ${width} ${height}`)
            .attr('preserveAspectRatio', 'xMinYMin meet')
            .classed('smts-overlay', true);
    },

    // Make group which holds all nodes and on which the zoomListener acts upon
    // @param {DOMElement} svgBase: The SVG element in which the new element
    // will be appended.
    // @return {DOMElement}: The new SVG element.
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
    // @param {number} width: Width of the tree.
    // @param {number} heigt: Height of the tree.
    // @return {d3.TreeLayout}: Layout used by d3 library, that represents the
    // tree.
    makeD3Tree: function(width, height) {
        // Width and height have swapped order
        return d3.layout.tree().size([height, width]);
    },

    // Make a d3 diagonal projection for use by the node paths
    // @return {d3.DiagonalProjection}: Projection used by the d3 library to
    // compute the positioning of links between nodes.
    makeD3Diagonal: function() {
        return d3.svg.diagonal().projection(node => [node.y, node.x]);
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NODES
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Populate nodes with elements and listeners, place them and assign styles
    // @param {DOMElement} svgGroup: Element containing all nodes. The nodes
    // will be filled with the actual visible elements (circles, diamonds,
    // etc.) and event listeners, placed in their correct position, and styles
    // will be applied.
    // @param {d3.Node[]} d3Nodes: List containing nodes data.
    populateNodes: function(svgGroup, d3Nodes) {
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
            .classed('smts-nodeSelected', node => node.equalAny(this.tree.selectedNodes))
            .attr('transform', `translate(${this.tree.root.y0}, ${this.tree.root.x0})`)
            .on('click', function(node) {
                smts.data.update(node, 'node');
                smts.tree.tree.setSelectedNodes([node]);
                smts.tree.updateSelectedNode(this); // `this` is the DOM element
            });

        // Add rhombi to OR nodes
        svgGroup.selectAll('.smts-nodeOr')
            .append('rect')
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
            .classed('smts-propagated', node => node.isStatusPropagated)
            .on('dblclick', node => this.requestPartitioning(node));

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
            .text(node => node.type === 'AND' ? node.solversAddresses.length : null);

        // Transition nodes to their new position.
        svgNodes.transition()
            .duration(this.TRANSITION_DURATION)
            .attr('transform', node => `translate(${node.y},${node.x})`);
    },

    // Update selected node
    // Remove previous selected nodes, select the given node and update events
    // and solvers tables to show only rows concerning that node.
    // @param {DOMElement} node: The node to be selected.
    updateSelectedNode: function(node) {
        // Remove previous selections
        d3.selectAll('.smts-nodeSelected')
            .classed('smts-nodeSelected', false)
            .selectAll('.smts-selected')
            .remove();

        // Select new node
        d3.select(node)
            .classed('smts-nodeSelected', true)
            .append('circle')
            .attr('r', this.NODE_SELECTED_RADIUS / this.scale)
            .classed('smts-selected', true);

        // Update events if 'Selected' tab is selected
        smts.events.update(this.tree.selectedNodes);
        smts.solvers.update(this.tree.selectedNodes);
    },

    // Request partitioning on given AND node
    // This works only if the instance is currently being solved and the node
    // is not partitioned yet.
    // @param {TreeManager.Node} node: The node to be partitioned by the solver
    // application.
    requestPartitioning: function(node) {
        let instanceName = smts.instances.getSelected();
        let nodePath = JSON.stringify(node.path);
        $.ajax({
            url: `/partition`,
            type: 'POST',
            data: {instanceName: instanceName, nodePath: nodePath},
            success: res => {}
        }, smts.tools.error)
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LINKS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Populate links between nodes, place them and assign styles
    // @param {DOMElement} svgGroup: Element containing all links. The links
    // will be placed in their correct position and styles will be applied.
    // @param {d3.Link[]} d3Links: List containing links data.
    populateLinks: function(svgGroup, d3Links) {

        let root = this.tree.root;

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
    // @param {number} scale: The new scale factor.
    setScale: function(scale) {
        this.scale = scale;
        this.scaleSelectedCircles();
    },

    // Scale selected node halo when zooming in or out
    scaleSelectedCircles: function() {
        let circles = document.querySelectorAll('circle.smts-selected');
        circles.forEach(circle => circle.setAttribute('r', (this.NODE_SELECTED_RADIUS / this.scale).toString()));
    },

    // Move frame to (x,y) coordinates
    // @param {number} x: New horizontal position.
    // @param {number} y: New vertical position.
    // @param {number} scale: New scale factor.
    // @param {d3.ZoomBehavior} zoomListener: Listener that manages the zoom.
    move: function(x, y, scale, zoomListener) {
        let position = `translate(${x}, ${y}) scale(${scale})`;

        d3.select('g')
            .attr('transform', position)
            .transition()
            .duration(this.TRANSITION_DURATION);

        this.setScale(scale);

        zoomListener.scale(scale);
        zoomListener.translate([x, y]);
    },

    // Move frame, centering on (x,y) coordinates
    // @param {number} x: New horizontal position.
    // @param {number} y: New vertical position.
    // @param {number} width: Width of the frame.
    // @param {number} height: Height of the frame.
    // @param {number} scale: New scale factor.
    // @param {d3.ZoomBehavior} zoomListener: Listener that manages the zoom.
    center: function(x, y, width, height, scale, zoomListener) {
        this.move(-y * scale + width / 2, -x * scale + height / 2, scale, zoomListener);
    },

    // Check if position is between bounds
    // The (x,y) coordinates must be in the rectangle (left, right, bottom,
    // top), with an error margin for the rectangle proportional to its size.
    // @param {number} x: The horizontal position.
    // @param {number} y: The vertical position.
    // @param {number} left: The rectangle left bound.
    // @param {number} right: The rectangle right bound.
    // @param {number} bottom: The rectangle bottom bound.
    // @param {number} top: The rectangle top bound.
    // @return {boolean}: `true` if (x,y) is in the rectangle (considering the
    // margin of error), `false` otherwise.
    isInBounds: function(x, y, left, right, bottom, top) {
        let errX = (right - left) * this.BOUNDS_ERROR_FACTOR;
        let errY = (top - bottom) * this.BOUNDS_ERROR_FACTOR;
        return left + errX <= x && x < right - errX && bottom + errY <= y && y < top - errY;
    },

    // Center tree to correct position
    // If the selected node is not already visible in the current frame (or if
    // it is visible, but really close to the border of the frame), then frame
    // is moved to center the selected node, otherwise the position is restored
    // as it was in the previous tree representation.
    // @param {number} width: Width of the frame.
    // @param {number} height: Height of the frame.
    // @param {TreeManager.Node} node: Node to be centered.
    // @param {d3.ZoomBehavior} zoomListener: Listener that manages the zoom.
    centerTree: function(width, height, node, zoomListener) {
        let position = this.getPosition();
        if (position) {
            let positionSelected = document.querySelector('circle.smts-selected').parentNode.getAttribute('transform');
            let translateSelected = this.getTranslate(positionSelected);
            let translateFrame = this.getTranslate(position);
            let scale = this.getScale(position) || zoomListener.scale();
            let x = translateSelected[0] * scale + translateFrame[0];
            let y = translateSelected[1] * scale + translateFrame[1];

            if (this.isInBounds(x, y, 0, width, 0, height)) {
                this.move(translateFrame[0], translateFrame[1], scale, zoomListener);
            }
            else {
                let scale = this.getScale(position) || zoomListener.scale();
                this.center(node.x0, node.y0, width, height, scale, zoomListener);
            }
        }
        else {
            this.center(node.x0, node.y0, width, height, zoomListener.scale(), zoomListener);
        }
    },


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OTHER
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Get width of tree level with the greatest number of nodes
    // Count total number of nodes in each level of depth, and return the
    // greatest.
    // @param {TreeManager.Tree} tree: The tree to be analyzed.
    // @return {number}: The max level width.
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

    // Parse a CSS `transform` and get `translate` coordinates
    // @param {string} position: The CSS `transform` attribute value.
    // Example: 'translate(12, -15) scale(1.3)'
    // @return {number[]}: An array containing the `translate` coordinates, or
    // `null` if the `translate` property is not present.
    // Example: [12, -15]
    getTranslate: function(position) {
        if (position && position.includes('translate')) {
            let values = position.match(/translate\(([^)]+)\)/)[1].split(',');
            return [parseFloat(values[0]), parseFloat(values[1])];
        }
        return null;
    },

    // Parse a CSS `transform` and get `scale` factor
    // @param {string} position: The CSS `transform` attribute value.
    // Example: 'translate(12, -15) scale(1.3)'
    // @return {number}: The value of the `scale` factor, or `null` if the
    // `scale` property is not present.
    // Example: 1.3
    getScale: function(position) {
        if (position && position.includes('scale')) {
            return parseFloat(position.match(/scale\(([^)]+)\)/)[1]);
        }
        return null;
    }
};