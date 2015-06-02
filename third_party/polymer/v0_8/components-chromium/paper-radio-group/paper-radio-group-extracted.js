
  Polymer({
    is: 'paper-radio-group',

    behaviors: [
      Polymer.IronA11yKeysBehavior
    ],

    hostAttributes: {
      role: 'radiogroup',
      tabindex: 0
    },

    properties: {
      /**
       * Fired when the selected element changes to user interaction.
       *
       * @event paper-radio-group-changed
       */

      /**
       * Gets or sets the selected element. Use the `name` attribute of the
       * <paper-radio-button> that should be selected.
       *
       * @attribute selected
       * @type String
       * @default null
       */

      selected: {
        type: String,
        value: null,
        notify: true,
        reflectToAttribute: true,
        observer: "_selectedChanged"
      }
    },

    keyBindings: {
      'left up': '_selectPrevious',
      'right down': '_selectNext',
    },

    _selectedChanged: function() {
      // TODO: This only needs to be async while a domReady event is unavailable.
      this.async(function() {
        this._selectIndex(this._valueToIndex(this.items, this.selected));
        this.fire('paper-radio-group-changed');
      });
    },

    _selectNext: function() {
      this.selected = this._nextNode();
    },

    _selectPrevious: function() {
      this.selected = this._previousNode();
    },

    /**
     * Returns an array of all items.
     *
     * @property items
     * @type array
     */
    get items() {
      return Polymer.dom(this.$.items).getDistributedNodes();
    },

    _nextNode: function() {
      var items = this.items;
      var index = this._selectedIndex;
      var newIndex = index;
      do {
        newIndex = (newIndex + 1) % items.length;
        if (newIndex === index) {
          break;
        }
      } while (items[newIndex].disabled);
      return this._valueForNode(items[newIndex]);
    },

    _previousNode: function() {
      var items = this.items;
      var index = this._selectedIndex;
      var newIndex = index;
      do {
        newIndex = (newIndex || items.length) - 1;
        if (newIndex === index) {
          break;
        }
      } while (items[newIndex].disabled);
      return this._valueForNode(items[newIndex]);
    },

    _selectIndex: function(index) {
      if (index == this._selectedIndex)
        return;

      var nodes = this.items;

      // If there was a previously selected node, deselect it.
      if (nodes[this._selectedIndex]) {
        nodes[this._selectedIndex].checked = false;
      }

      // Select a new node.
      nodes[index].checked = true;
      nodes[index].focus();
      this._selectedIndex = index;
    },

    _valueForNode: function(node) {
      return node["name"] || node.getAttribute("name");
    },

    // Finds an item with value == |value| and return its index.
    _valueToIndex: function(items, value) {
      for (var index = 0, node; (node = items[index]); index++) {
        if (this._valueForNode(node) == value) {
          return index;
        }
      }
      // If no item found, the value itself is probably the index.
      return value;
    }
  });
