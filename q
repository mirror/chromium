[1mdiff --git a/third_party/WebKit/Source/core/layout/LayoutTableCell.cpp b/third_party/WebKit/Source/core/layout/LayoutTableCell.cpp[m
[1mindex 753fe7324614..e3153ee7bbc3 100644[m
[1m--- a/third_party/WebKit/Source/core/layout/LayoutTableCell.cpp[m
[1m+++ b/third_party/WebKit/Source/core/layout/LayoutTableCell.cpp[m
[36m@@ -256,7 +256,7 @@[m [mvoid LayoutTableCell::ComputeIntrinsicPadding(int collapsed_height,[m
                                               int row_height,[m
                                               EVerticalAlign vertical_align,[m
                                               SubtreeLayoutScope& layouter) {[m
[31m-  row_height += collapsed_height;[m
[32m+[m[32m  // row_height += collapsed_height;[m
 [m
   int old_intrinsic_padding_before = IntrinsicPaddingBefore();[m
   int old_intrinsic_padding_after = IntrinsicPaddingAfter();[m
[36m@@ -296,7 +296,10 @@[m [mvoid LayoutTableCell::ComputeIntrinsicPadding(int collapsed_height,[m
                                 logical_height_without_intrinsic_padding -[m
                                 intrinsic_padding_before;[m
   SetIntrinsicPaddingBefore(intrinsic_padding_before);[m
[31m-  SetIntrinsicPaddingAfter(intrinsic_padding_after - collapsed_height);[m
[32m+[m[32m  SetIntrinsicPaddingAfter(intrinsic_padding_after); //- collapsed_height);[m
[32m+[m
[32m+[m[32m  //LOG(ERROR)<<"This is after: "<<intrinsic_padding_after - collapsed_height;[m
[32m+[m
 [m
   // FIXME: Changing an intrinsic padding shouldn't trigger a relayout as it[m
   // only shifts the cell inside the row but doesn't change the logical height.[m
[1mdiff --git a/third_party/WebKit/Source/core/layout/LayoutTableCell.h b/third_party/WebKit/Source/core/layout/LayoutTableCell.h[m
[1mindex 4b31bd65663f..7636a9b7492f 100644[m
[1m--- a/third_party/WebKit/Source/core/layout/LayoutTableCell.h[m
[1m+++ b/third_party/WebKit/Source/core/layout/LayoutTableCell.h[m
[36m@@ -167,9 +167,11 @@[m [mclass CORE_EXPORT LayoutTableCell final : public LayoutBlockFlow {[m
   int LogicalHeightForRowSizing() const {[m
     // FIXME: This function does too much work, and is very hot during table[m
     // layout![m
[32m+[m[32m    // LOG(ERROR)<<"intrinsic padding before: "<<IntrinsicPaddingBefore();[m
[32m+[m[32m    // LOG(ERROR)<<"intrinsic padding after: "<<IntrinsicPaddingAfter();[m
     int adjusted_logical_height =[m
         PixelSnappedLogicalHeight() -[m
[31m-        (IntrinsicPaddingBefore() + IntrinsicPaddingAfter());[m
[32m+[m[32m        (IntrinsicPaddingBefore() + std::max(0, IntrinsicPaddingAfter()));[m
     int style_logical_height = LogicalHeightFromStyle();[m
     return max(style_logical_height, adjusted_logical_height);[m
   }[m
[1mdiff --git a/third_party/WebKit/Source/core/layout/LayoutTableSection.cpp b/third_party/WebKit/Source/core/layout/LayoutTableSection.cpp[m
[1mindex 66de61fa9afb..900e363cfb62 100644[m
[1m--- a/third_party/WebKit/Source/core/layout/LayoutTableSection.cpp[m
[1m+++ b/third_party/WebKit/Source/core/layout/LayoutTableSection.cpp[m
[36m@@ -833,14 +833,13 @@[m [mint LayoutTableSection::CalcRowLogicalHeight() {[m
   // it spans. Otherwise we'd need to refragment afterwards.[m
   unsigned index_of_first_stretchable_row = 0;[m
 [m
[31m-  is_any_row_collapsed = false;[m
[32m+[m[32m  is_any_row_collapsed_ = false;[m
 [m
   for (unsigned r = 0; r < grid_.size(); r++) {[m
     grid_[r].baseline = -1;[m
     int baseline_descent = 0;[m
 [m
[31m-    if (RowHasVisibilityCollapse(r) && !is_any_row_collapsed)[m
[31m-      is_any_row_collapsed = true;[m
[32m+[m[32m    is_any_row_collapsed_ |= RowHasVisibilityCollapse(r) && !is_any_row_collapsed_;[m
 [m
     if (state.IsPaginated() && grid_[r].row)[m
       row_pos_[r] += grid_[r].row->PaginationStrut().Ceil();[m
[36m@@ -923,7 +922,7 @@[m [mint LayoutTableSection::CalcRowLogicalHeight() {[m
   // This is because the distribution calculations should be as if the row were[m
   // not collapsed. First, all rows' collapsed heights are set. After, row[m
   // positions are adjusted accordingly.[m
[31m-  if (is_any_row_collapsed) {[m
[32m+[m[32m  if (is_any_row_collapsed_) {[m
     row_collapsed_height_.resize(grid_.size());[m
     for (unsigned r = 0; r < grid_.size(); r++) {[m
       if (RowHasVisibilityCollapse(r)) {[m
[36m@@ -1220,7 +1219,7 @@[m [mvoid LayoutTableSection::LayoutRows() {[m
 [m
       // Calculate total collapsed height affecting one cell.[m
       int collapsed_height = 0;[m
[31m-      if (is_any_row_collapsed) {[m
[32m+[m[32m      if (is_any_row_collapsed_) {[m
         unsigned end_row = cell->RowSpan() + r;[m
         for (unsigned spanning = r; spanning < end_row; spanning++) {[m
           collapsed_height += row_collapsed_height_[spanning];[m
[1mdiff --git a/third_party/WebKit/Source/core/layout/LayoutTableSection.h b/third_party/WebKit/Source/core/layout/LayoutTableSection.h[m
[1mindex c4d7fbbf7b6c..395efdd8c817 100644[m
[1m--- a/third_party/WebKit/Source/core/layout/LayoutTableSection.h[m
[1m+++ b/third_party/WebKit/Source/core/layout/LayoutTableSection.h[m
[36m@@ -425,7 +425,7 @@[m [mclass CORE_EXPORT LayoutTableSection final : public LayoutTableBoxComponent {[m
   Vector<int> row_collapsed_height_;[m
 [m
   // Whether any row in the table section is or has been collapsed.[m
[31m-  bool is_any_row_collapsed;[m
[32m+[m[32m  bool is_any_row_collapsed_;[m
 [m
   // The current insertion position in the grid.[m
   // The position is used when inserting a new cell into the section to[m
[1mdiff --git a/third_party/WebKit/Source/core/layout/TableLayoutAlgorithmAuto.cpp b/third_party/WebKit/Source/core/layout/TableLayoutAlgorithmAuto.cpp[m
[1mindex 9d3fae5ffc40..8fd521a3e276 100644[m
[1m--- a/third_party/WebKit/Source/core/layout/TableLayoutAlgorithmAuto.cpp[m
[1m+++ b/third_party/WebKit/Source/core/layout/TableLayoutAlgorithmAuto.cpp[m
[36m@@ -169,6 +169,8 @@[m [mvoid TableLayoutAlgorithmAuto::FullRecalc() {[m
   unsigned current_column = 0;[m
   for (LayoutTableCol* column = table_->FirstColumn(); column;[m
        column = column->NextColumn()) {[m
[32m+[m[32m    LOG(ERROR)<<"current column: "<<current_column;[m
[32m+[m[32m    LOG(ERROR)<<"collapse? "<<(column->Style()->Visibility() == EVisibility::kCollapse);[m
     if (column->IsTableColumnGroupWithColumnChildren()) {[m
       group_logical_width = column->Style()->LogicalWidth();[m
     } else {[m
