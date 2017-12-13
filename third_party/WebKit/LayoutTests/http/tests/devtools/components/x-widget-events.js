// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`This tests that events are properly propagated through XWidget hierarchy.\n`);

  var nextWidth = 0;

  var TestWidget = class {
    constructor(widgetName) {
      this.widget = document.createElement('x-widget');
      this.widget.style.width = (++nextWidth) + 'px';
      this.widget.style.height = '3px';
      this.widget.style.flex = 'auto';
      this.element = this.widget;

      this._widgetName = widgetName;
      this._onShowCount = 0;
      this._onHideCount = 0;

      this.widget.onShow = () => {
        TestRunner.assertEquals(this._onShowCount, this._onHideCount);
        ++this._onShowCount;

        TestRunner.addResult('  ' + this._widgetName + '.wasShown()');
        if (this.showOnWasShown)
          this.showOnWasShown.show(this.showRoot || this.widget);
        if (this.hideOnWasShown)
          this.hideOnWasShown.detach();
        if (this.resizeOnWasShown)
          this.resizeOnWasShown.doResize();
      };

      this.widget.onHide = () => {
        ++this._onHideCount;
        TestRunner.assertEquals(this._onShowCount, this._onHideCount);

        TestRunner.addResult('  ' + this._widgetName + '.willHide()');
        if (this.showOnWillHide)
          this.showOnWillHide.show(this.widget);
        if (this.detachOnWillHide)
          this.detachOnWillHide.detach();
      };

      this.widget.onResize = () => {
        TestRunner.addResult('  ' + this._widgetName + '.onResize()');
      };

      TestRunner.addResult(this._widgetName + '()');
    }

    show(parentElement) {
      TestRunner.addResult(this._widgetName + '.show()');
      parentElement.appendChild(this.widget);
    }

    detach() {
      TestRunner.addResult(this._widgetName + '.detach()');
      this.widget.remove();
    }

    doResize() {
      TestRunner.addResult(this._widgetName + '.doResize()');
      this.widget.style.width = (++nextWidth) + 'px';
    }

    markAsRoot() {
    }

    setHideOnDetach() {
    }

    hideWidget() {
    }

    detachChildWidgets() {
    }
  };

  function raf(next) {
    window.requestAnimationFrame(() => {
      window.requestAnimationFrame(next);
    });
  }

  TestRunner.runTestSuite([
    function testShowWidget(next) {
      var widget = new TestWidget('Widget');
      widget.show(UI.inspectorView.element);
      widget.detach();
      raf(next);
    },

    function testAppendViaDOM(next) {
      try {
        var widget = new TestWidget('Widget');
        document.body.appendChild(widget.element);
      } catch (e) {
        TestRunner.addResult(e);
      }
      raf(next);
    },

    function testInsertViaDOM(next) {
      try {
        var widget = new TestWidget('Widget');
        document.body.insertBefore(widget.element, null);
      } catch (e) {
        TestRunner.addResult(e);
      }
      raf(next);
    },

    function testAttachToOrphanNode(next) {
      try {
        var widget = new TestWidget('Widget');
        var div = document.createElement('div');
        widget.show(div);
      } catch (e) {
        TestRunner.addResult(e);
      }
      raf(next);
    },

    function testImmediateParent(next) {
      var parentWidget = new TestWidget('Parent');
      var childWidget = new TestWidget('Child');
      childWidget.show(parentWidget.element);
      if (childWidget._parentWidget === parentWidget)
        TestRunner.addResult('OK');
      else
        TestRunner.addResult('FAILED');
      raf(next);
    },

    function testDistantParent(next) {
      var parentWidget = new TestWidget('Parent');
      var div = document.createElement('div');
      parentWidget.element.appendChild(div);
      var childWidget = new TestWidget('Child');
      childWidget.show(div);

      if (childWidget._parentWidget === parentWidget)
        TestRunner.addResult('OK');
      else
        TestRunner.addResult('FAILED');
      raf(next);
    },

    function testEvents(next) {
      var parentWidget = new TestWidget('Parent');
      parentWidget.markAsRoot();
      var childWidget = new TestWidget('Child');
      parentWidget.show(document.body);

      parentWidget.doResize();
      childWidget.show(parentWidget.element);
      parentWidget.doResize();
      parentWidget.detach();
      parentWidget.show(document.body);
      childWidget.detach();
      parentWidget.detach();
      raf(next);
    },

    function testEventsHideOnDetach(next) {
      var parentWidget = new TestWidget('Parent');
      var childWidget = new TestWidget('Child');
      childWidget.setHideOnDetach();
      parentWidget.show(UI.inspectorView.element);

      parentWidget.doResize();
      childWidget.show(parentWidget.element);
      parentWidget.doResize();
      parentWidget.detach();
      parentWidget.show(UI.inspectorView.element);
      childWidget.detach();
      parentWidget.detach();
      raf(next);
    },

    function testWidgetCounter(next) {
      var parentWidget = new TestWidget('Parent');
      parentWidget.show(UI.inspectorView.element);

      var childWidget = new TestWidget('Child');
      childWidget.show(parentWidget.element);
      TestRunner.addResult('  widget counter: ' + parentWidget.element.__widgetCounter);

      var childWidget2 = new TestWidget('Child 2');
      childWidget2.show(parentWidget.element);
      TestRunner.addResult('  widget counter: ' + parentWidget.element.__widgetCounter);

      childWidget.detach();
      TestRunner.addResult('  widget counter: ' + parentWidget.element.__widgetCounter);

      childWidget2.detach();
      TestRunner.addResult('  widget counter: ' + parentWidget.element.__widgetCounter);

      raf(next);
    },

    function testRemoveChild(next) {
      var parentWidget = new TestWidget('Parent');
      parentWidget.show(UI.inspectorView.element);

      var childWidget = new TestWidget('Child');
      childWidget.show(parentWidget.element);
      try {
        parentWidget.element.removeChild(childWidget.element);
      } catch (e) {
        TestRunner.addResult(e);
      }
      raf(next);
    },

    function testImplicitRemoveChild(next) {
      var parentWidget = new TestWidget('Parent');
      var div = document.createElement('div');
      parentWidget.element.appendChild(div);

      var childWidget = new TestWidget('Child');
      childWidget.show(div);

      try {
        parentWidget.element.removeChild(div);
      } catch (e) {
        TestRunner.addResult(e);
      }
      raf(next);
    },

    function testRemoveChildren(next) {
      var parentWidget = new TestWidget('Parent');
      var childWidget = new TestWidget('Child');
      childWidget.show(parentWidget.element);
      parentWidget.element.appendChild(document.createElement('div'));
      try {
        parentWidget.element.removeChildren();
      } catch (e) {
        TestRunner.addResult(e);
      }
      raf(next);
    },

    function testImplicitRemoveChildren(next) {
      var parentWidget = new TestWidget('Parent');
      var div = document.createElement('div');
      parentWidget.element.appendChild(div);

      var childWidget = new TestWidget('Child');
      childWidget.show(div);

      try {
        parentWidget.element.removeChildren();
      } catch (e) {
        TestRunner.addResult(e);
      }
      raf(next);
    },

    function testShowOnWasShown(next) {
      var parentWidget = new TestWidget('Parent');
      parentWidget.showOnWasShown = new TestWidget('Child');
      parentWidget.show(UI.inspectorView.element);
      parentWidget.detach();
      raf(next);
    },

    function testShowNestedOnWasShown(next) {
      var topWidget = new TestWidget('Top');
      var middleWidget = new TestWidget('Middle');
      var bottomWidget = new TestWidget('Bottom');
      middleWidget.show(topWidget.element);
      topWidget.showOnWasShown = bottomWidget;
      topWidget.showRoot = middleWidget.element;
      topWidget.show(UI.inspectorView.element);
      topWidget.detach();
      raf(next);
    },

    function testDetachOnWasShown(next) {
      var parentWidget = new TestWidget('Parent');
      var childWidget = new TestWidget('Child');
      childWidget.show(parentWidget.element);
      parentWidget.detachOnWasShown = childWidget;
      parentWidget.show(UI.inspectorView.element);
      parentWidget.detach();
      raf(next);
    },

    function testShowOnWillHide(next) {
      var parentWidget = new TestWidget('Parent');
      var childWidget = new TestWidget('Child');
      parentWidget.show(UI.inspectorView.element);
      childWidget.show(parentWidget.element);
      parentWidget.showOnWillHide = childWidget;
      parentWidget.detach();
      raf(next);
    },

    function testDetachOnWillHide(next) {
      var parentWidget = new TestWidget('Parent');
      var childWidget = new TestWidget('Child');
      parentWidget.show(UI.inspectorView.element);
      childWidget.show(parentWidget.element);
      parentWidget.detachOnWillHide = childWidget;
      parentWidget.detach();
      raf(next);
    },

    function testShowDetachesFromPrevious(next) {
      var parentWidget1 = new TestWidget('Parent1');
      var parentWidget2 = new TestWidget('Parent2');
      var childWidget = new TestWidget('Child');
      parentWidget1.show(UI.inspectorView.element);
      parentWidget2.show(UI.inspectorView.element);
      childWidget.show(parentWidget1.element);
      childWidget.show(parentWidget2.element);
      raf(next);
    },

    function testResizeOnWasShown(next) {
      var parentWidget = new TestWidget('Parent');
      var childWidget = new TestWidget('Child');
      childWidget.show(parentWidget.element);
      parentWidget.resizeOnWasShown = childWidget;
      parentWidget.show(UI.inspectorView.element);
      parentWidget.detach();
      raf(next);
    },

    function testReparentWithinWidget(next) {
      var parentWidget = new TestWidget('Parent');
      parentWidget.show(UI.inspectorView.element);
      var childWidget = new TestWidget('Child');
      var container1 = parentWidget.element.createChild('div');
      var container2 = parentWidget.element.createChild('div');
      childWidget.show(container1);
      childWidget.show(container2);
      raf(next);
    },

    function testDetachChildWidgetsRemovesHiddenChildren(next) {
      var parentWidget = new TestWidget('Parent');
      var visibleChild = new TestWidget('visibleChild');
      var hiddenChild = new TestWidget('hiddenChild');
      parentWidget.show(UI.inspectorView.element);
      visibleChild.show(parentWidget.element);
      hiddenChild.show(parentWidget.element);
      hiddenChild.hideWidget();
      parentWidget.detachChildWidgets();
      var count = parentWidget.element.childElementCount;
      TestRunner.addResult(`Parent element has ${count} child elements`);
      raf(next);
    }
  ]);
})();
