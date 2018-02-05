#ifndef ASH_CONTENT_CARD_VIEW_H_
#define ASH_CONTENT_CARD_VIEW_H_

#include "ash/content/ash_with_content_export.h"
#include "base/macros.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

class GURL;

namespace gfx {
class Rect;
}  // namespace gfx

namespace content {
class BrowserContext;
}  // namespace content.

namespace ash {

class ASH_WITH_CONTENT_EXPORT CardView : public views::WidgetDelegate,
                                         public views::Widget {
 public:
  static void ShowCard(content::BrowserContext* context,
                       const GURL& url,
                       const gfx::Rect& bounds);
  CardView(content::BrowserContext* context,
           const GURL& url,
           const gfx::Rect& bounds);
  ~CardView() override = default;

  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CardView);
};

}  // namespace ash

#endif  // ASH_CONTENT_CARD_VIEW_H_
