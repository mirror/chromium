#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"

// TODO(crbug.com/435048): Add UIImageView with user's avatar.
// TODO(crbug.com/435048): Animate appearance and disappearance.

// UIViewController for the notification about auto sign-in.
@interface NotifyUserAutoSigninViewController : UIViewController

- (instancetype)initWithUserId:(NSString*)userId;

// Username, corresponding to Credential.id field in JS.
@property(copy, atomic) NSString* userId;

@end
