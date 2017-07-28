#import "ios/clean/chrome/app/steps/provider_initializer.h"

#import "ios/chrome/app/startup/provider_registration.h"
#include "ios/chrome/browser/application_context.h"
#import "ios/clean/chrome/app/steps/step_features.h"

@protocol StepContext;
#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif
@implementation ProviderInitializer
- (instancetype)init {
  if ((self = [super init])) {
    self.providedFeature = step_features::kProviders;
  }
  return self;
}

- (void)runFeature:(NSString*)feature withContext:(id<StepContext>)context {
[ProviderRegistration registerProviders];

 }
 
@end
