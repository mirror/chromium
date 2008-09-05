// Copyright 2004 Google Inc.
// All Rights Reserved.
//
// msamuel@google.com

// Functions for rearranging overlapping regions.
// Public API
// - rearrangeStackHorizontally: the default stacking scheme
// - rearrangeCardLayout: an alternate stacking scheme that uses a z-ordering as
//   well

// names of fields in Rect.
var OL_X_AXIS = 'x';
var OL_Y_AXIS = 'y';
var OL_X_EXTENT = 'w';
var OL_Y_EXTENT = 'h';

if (ASSERTS_ON) {
  (function() {
    var protoRect = new Rect(0, 0, 0, 0, undefined);
    AssertTrue(OL_X_AXIS in protoRect);
    AssertTrue(OL_Y_AXIS in protoRect);
    AssertTrue(OL_X_EXTENT in protoRect);
    AssertTrue(OL_Y_EXTENT in protoRect);
  })();
}

/** rearranges overlapping rectangles so that they do not overlap by
  * repositioning them in subcolumns.  This function is idempotent.
  * @param rects objects with x, y, w, h attributes.  Modified in place.
  *   x and y are upper left corners, while w and h are width and height
  *   respectively.  The rects must occupy discrete columns.
  */
function rearrangeStackHorizontally(rects) {
  rearrangeStack(rects, OL_X_AXIS);
}

/** rearranges overlapping rectangles so that they do not overlap by
  * repositioning them in subcolumns.
  *
  * <h3>Input</h3>
  * <p>We have a bunch of rectangles laid out on a 2-D page, and we he have a
  * major axis and a minor axis, so rectangles either left to right,
  * top to bottom, as in the Roman alphabet (majorAxis:Y);
  * or top to bottom, left to right, as in traditional Chinese (majorAxis:X).
  * The reading order corresponds to time-order when talking about events.
  * Week view is laid out as per Chinese reading order, and month view follows
  * Roman.
  *
  * <p>The goal is an arrangement of rectangles where no rectangles overlap,
  * the output rectangles span the same range in their minor axis, and a subset
  * of their range along the major axis.</p>
  *
  *
  * <h3>Assumptions</h3>
  * <p>The rectangles are already divided into disjoint regions along the
  * major axis, so there are no rectangles R1 and R2 such that R1 and R2 do
  * not have the same span along the major, and R1's and R2's spans intersect.
  * </p>
  *
  *
  * <h3>Algorithm</h3>
  * <p>This is achieved by the following algorithm which orders rectangles,
  * iteratively finds the minimum set of rectangles where each rectangle only
  * overlaps with others in the set, and then divides up the major axis so that
  * no rectangles overlaps others.</p>
  * <ol>
  *
  * <li>First we sort rectangles in reading order (see Roman/Chinese above).
  * <br>The sort is stable so that, below, earlier occuring items with the same
  * position in reading order receive the first slots making them more likely
  * to escape culling.
  *
  * <li>Until we've checked all rectangles
  *   <ol>
  *   <li>Pick the first rectangle in order that hasn't been resolved
  *
  *   <li>Add that to the set of rectangles to resolve.
  *
  *   <li>Walk from that point in the list of rectangles forward, adding
  *   rectangles until we encounter one that doesn't intersect any rectangle
  *   in the set.
  *
  *   <br>This is efficient because we know that we're traversing rectangles in
  *   "reading order", and that the rectangles are in disjoint "columns" along
  *   the minor axis.  These two assumptions mean that it's trivial to come up
  *   with a bounding box for the region which is the minor axis min of the
  *   first item added to the minor axis max of all elements in the set.
  *
  *   <li>Now that we know the set of overlapping rectangles, we can assign each
  *   rectangle to a subset of its "column", its span along the major axis.
  *
  *   <br>For each rectangle R in the overlapping set, ordered by reading order
  *     <ol>
  *     <li>If there are rectangles in slots that don't overlap R, mark their
  *     slot free.
  *     <li>Assign R to the first free slot, creating one if necessary
  *     <li>Mark the slot busy
  *     </ol>
  *
  *   <li>We now have an initial slot assignment, but there may be wasted space,
  *   as in the case
  *   <blockquote><pre>
  *   A B C
  *   A B C
  *       C
  *   D   C</pre><small><i>major axis is x</i></small></blockquote>
  *   D could expand to occupy two slots, using more of the available space.
  *
  *   <br>We again walk the list of overlapping rectangles, keeping track of
  *   free-slots in the same way as above, but for each slot, we also keep track
  *   of the position where it was last occupied so that we can tell when
  *   assigning a slot whether or not it could span slots to the right/down.
  *   <br>For each rectangle R in the overlapping set, ordered by reading order
  *     <ol>
  *     <li>If there are rectangles in slots that don't overlap R, mark their
  *     slot free, and store their endpoint (minor-axis max) in extent[slot].
  *     <li>If R was assigned slot S, assign it the end slot T, and mark S
  *     occupied.
  *     <li>Greedily expand into slots where there is space available.
  *     <tt>while (T < num_slots && free[T] && extent[T] < R's minor-axis min)
  *       T +=1</tt>
  *     </ol>
  *   This does not guarantee the maximum area covered, but is a good
  *   approximation in practice.
  *
  *   <li>Finally, we divvy the "column" into equally wide "sub-columns", and
  *   update each rectangles major axis bounds to span its slots.
  * </ol>
  *
  *
  * <h3>Output</h3>
  * <p>Since a rectangle may span multiple columns, the assumptions on input
  * do not apply to the output.</p>
  *
  *
  * @param rects objects with x, y, w, h attributes.  Modified in place.
  *   x and y are upper left corners, while w and h are width and height
  *   respectively.  The rects must occupy discrete columns.
  * @param majorAxis the most significant axis along which time progresses.
  *   In week view, time progresses by hour along the y and days along the x,
  *   so x is the majorAxis.
  * @param opt_numSlots if specified, there will be a fixed number of slots,
  *   and if the rectangles cannot fit into the given number of slots,
  *   some will be culled.
  * @param opt_numReserved the number of reserved slots out of opt_numSlots
  * @param opt_cullLimit if specified, then when the number of slots exceeds
  *   opt_numSlots, then slots will be culled back to the cull limit.  This
  *   allows us to limit the slots to, say 4, but if there are more than 4, then
  *   call back to 3 leaving a blank slot at the bottom for a "More stuff"
  *   indicator.
  * @param opt_slotSize is the size of a slot. If this is specified, rects will
  *   be aligned at the start of the majorAxis and all extra space along the
  *   major axis will be at the end.
  * @param opt_sorted if this is true, then the rects are already sorted
  */
function rearrangeStack(rects, majorAxis, opt_numSlots, opt_numReserved,
                        opt_cullLimit, opt_slotSize, opt_sorted) {
  AssertType(rects, Array);
  AssertTrue(majorAxis == OL_X_AXIS || majorAxis == OL_Y_AXIS);

  opt_cullLimit = (opt_cullLimit === undefined) ? opt_numSlots : opt_cullLimit;

  // since this can be used to stack object in either rows (month view) or
  // columns (week view), figure out which axes we're dealing with by looking
  // at the majorAxis param.
  var minorAxis = OL_X_AXIS == majorAxis ? OL_Y_AXIS : OL_X_AXIS;
  var majorExtent = OL_X_AXIS == majorAxis ? OL_X_EXTENT : OL_Y_EXTENT;
  var minorExtent = OL_X_AXIS == minorAxis ? OL_X_EXTENT : OL_Y_EXTENT;
  opt_numReserved = opt_numReserved || 0;

  // sort rects by position such that rect[i] < rect[j] iff
  // ol[i][majorAxis] < ol[j][majorAxis] ||
  // (ol[i][majorAxis] == ol[j][majorAxis] &&
  //  (ol[i][minorAxis] < ol[j][minorAxis] ||
  //   (ol[i][minorAxis] == ol[j][minorAxis] &&
  //    ol[i][minorExtent] < ol[j][minorExtent])))
  var cmp = OL_X_AXIS == majorAxis ? rectComparatorX_ : rectComparatorY_;

  if (opt_sorted !== true) {
    rects.sort(cmp);
  }

  var r;
  for (var i = 0; i < rects.length; i += 1) {
    var end = -1; // -1 or the last overlay overlapping rects[i]

    // the bottom point on the minor axis of the last event
    var lastMinorEnd = rects[i][minorAxis] + rects[i][minorExtent];

    for (var j = i + 1; j < rects.length; j += 1) {
      r = rects[j];
      if (r[majorAxis] !== rects[i][majorAxis]
          || lastMinorEnd <= r[minorAxis]) {
        break;
      }
      end = j;
      if (lastMinorEnd < r[minorAxis] + r[minorExtent]) {
        // lastMinorEnd is used to check whether subsequent rectangles overlap
        // with any rectangles in [i, end].
        lastMinorEnd = r[minorAxis] + r[minorExtent];
      }
    }

    if (end >= 0) {
      var nOverlapping = end - i + 1;
      var nSlots = 0;
      // upper bound on # slots
      var slotUsage = bs_create(nOverlapping);
      // slot assigned per overlapping rect
      var assignedSlots = zeroedArray_(nOverlapping);
      // true while a rectangle is using it's slot
      var occupying = bs_create(nOverlapping);

      var k, m, s;
      var slot;
      var r2;
      for (k = 0; k < nOverlapping; ++k) {
        r = rects[i + k];

        // free up any slots no longer used
        // nSlots is small enough that we probably won't benefit from using a
        // heap
        for (m = -1; (m = bs_nextSetBit(occupying, m + 1)) >= 0;) {
          r2 = rects[i + m];
          if (r[minorAxis] >= r2[minorAxis] + r2[minorExtent]) {
            // free up r2's slot
            bs_clearBit(occupying, m);
            bs_clearBit(slotUsage, assignedSlots[m]);
          }
        }

        // find a free slot
        slot = bs_nextClearBit(slotUsage, opt_numReserved);
        bs_setBit(slotUsage, slot);
        assignedSlots[k] = slot;
        nSlots = Math.max(slot + 1, nSlots);

        bs_setBit(occupying, k);
      }

      // per overlapping rectangle, the number of extra slots wide it occupies.
      var extraSlots = zeroedArray_(nOverlapping);

      if (nSlots > 2
          // we do not expand when we have a fixed number of slots presumably
          // because all rectangles are supposed to have the same size
          && !opt_numSlots) {

        // Try to use any unclaimed space:
        // This may occur when you have the following situation
        // XY       XY
        // XYZ      XYZ
        // X Z      X Z
        //   Z  -->   Z
        // W Z      WWZ
        // W Z      WWZ
        // The space between W and Z is not being used, and W can be safely
        // expanded to fill two slots.

        bs_clearAll(occupying);

        // compute a contemporaries graph
        // contemporaries[s + k * nSlots] is true iff slot s is used within
        // rects[k + i]'s range
        var slotUser = zeroedArray_(nSlots);

        var contemporaries = bs_create(nSlots * nOverlapping);
        for (k = 0; k < nOverlapping; ++k) {
          r = rects[i + k];
          // free up any slots no longer used
          for (m = -1; (m = bs_nextSetBit(occupying, m + 1)) >= 0;) {
            r2 = rects[i + m];
            if (r[minorAxis] >= (r2[minorAxis] + r2[minorExtent])) {
              // free up r2's slot
              bs_clearBit(occupying, m);
              slotUser[assignedSlots[m]] = 0;
            }
          }

          slot = assignedSlots[k];
          slotUser[slot] = k + 1;
          bs_setBit(occupying, k);

          // update contemporaries
          for (s = 0; s < nSlots; s++) {
            if (slotUser[s]) {
              bs_setBit(contemporaries, slot + (slotUser[s] - 1) * nSlots);
              bs_setBit(contemporaries, s + k * nSlots);
            }
          }
        }

        // widen where appropriate.  The slot assignment algorithm assigns slots
        // such that we'll never be able to expand left.  Since we don't
        // expand left, we can expand right without updating contemporaries
        // since we don't have to worry about expanding later rectangles left
        // into the space we just expanded right into.
        for (k = 0; k < nOverlapping; ++k) {
          slot = assignedSlots[k];
          while ((slot += 1) < nSlots) {
            if (bs_get(contemporaries, slot + k * nSlots)) { break; }
          }
          extraSlots[k] = slot - assignedSlots[k] - 1;
        }
      }

      // reposition overlapping rectangles based on slot assignments
      var majorStart = rects[i][majorAxis];
      var slotSize;
      if (opt_slotSize) {
        slotSize = opt_slotSize;
      } else {
        slotSize = rects[i][majorExtent] / (opt_numSlots || nSlots);
      }
      var localCullLimit = opt_numSlots && nSlots > opt_numSlots ?
                           opt_cullLimit : Infinity;
      for (k = 0; k < nOverlapping; ++k) {
        slot = assignedSlots[k];
        r = rects[i + k];

        // cull any that don't fit within the given number of slots
        if (slot >= localCullLimit) {
          r[majorExtent] = 0;
          r.culled = true;
          continue;
        }

        // calc integral positions for columns without assigning the whole
        // remainder to first or last slot
        var off0 = Math.round(slot * slotSize);
        var off1;
        if (opt_slotSize) {
          off1 = off0 + opt_slotSize;
        } else {
          off1 = Math.round((slot + extraSlots[k] + 1) * slotSize);
        }
        r[majorAxis] = majorStart + off0;
        r[majorExtent] = off1 - off0;
      }

      // skip overlapping items which have already been recomputed in master
      // loop.  The loop incrementer will go to end + 1
      i = end;
    } else if (opt_numSlots) {
      // resize if we've got fixed number of slots
      r = rects[i];
      var slotSize;
      if (opt_slotSize) {
        slotSize = opt_slotSize;
      } else {
        slotSize = r[majorExtent] / opt_numSlots;
      }
      var off0 = Math.round(opt_numReserved * slotSize),
          off1 = Math.round((opt_numReserved + 1) * slotSize);
      r[majorAxis] += off0;
      r[majorExtent] = off1 - off0;
    }
  }
}

/** some common setup for the rearrange functions.
  * @param a an object with x, y, w, h attributes.
  *   x and y are upper left corners, while w and h are width and height
  *   respectively.
  * @param b an object with structure similar to a.
  * @private
  */
function rectComparatorX_(a, b) {
  // order by column, and then by position within the column.
  return (a.x - b.x) || (a.y - b.y) || (b.h - a.h);
}

/** some common setup for the rearrange functions.
  * @param a an object with x, y, w, h attributes.
  *   x and y are upper left corners, while w and h are width and height
  *   respectively.
  * @param b an object with structure similar to a.
  * @private
  */
function rectComparatorY_(a, b) {
  // order by row, and then by position within the row.
  return (a.y - b.y) || (a.x - b.x) || (b.w - a.w);
}

/**
 * returns an Array of length n with all values set to 0.
 * @private
 */
function zeroedArray_(n) {
  var arr = [];
  while (n) { arr[--n] = 0; }
  return arr;
}
