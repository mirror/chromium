#include "ash/content/card_view.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/webview/webview.h"
#include "url/gurl.h"

namespace ash {

// static
void CardView::ShowCard(content::BrowserContext* context,
                        const GURL& url,
                        const gfx::Rect& bounds) {
  auto* card_view = new CardView(context, url, bounds);
  card_view->GetWidget()->Show();
}

CardView::CardView(content::BrowserContext* context,
                   const GURL& url,
                   const gfx::Rect& bounds) {
  auto* web_view = new views::WebView(context);
  web_view->LoadInitialURL(url);
  web_view->SetBounds(0, 0, bounds.width(), bounds.height());
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.delegate = this;
  params.parent = Shell::GetContainer(Shell::GetPrimaryRootWindow(),
                                      kShellWindowId_AlwaysOnTopContainer);
  Init(params);
  GetNativeWindow()->SetBounds(bounds);
  views::Widget::GetContentsView()->AddChildView(web_view);
}

views::Widget* CardView::GetWidget() {
  return this;
}

const views::Widget* CardView::GetWidget() const {
  return this;
}

}  // namespace ash
