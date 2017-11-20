// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"

#import <objc/runtime.h>
#include <memory>

#import "base/ios/crb_protocol_observers.h"
#import "base/logging.h"
#import "base/mac/foundation_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Constructs an NSInvocation that will be used for repeated execution of
// |selector|. |selector| must return void and take exactly one argument; it is
// an error otherwise.
NSInvocation* InvocationForBroadcasterProperty(NSString* property,
                                               NSString* type) {
  NSMethodSignature* method = [NSMethodSignature
      signatureWithObjCTypes:[type cStringUsingEncoding:NSASCIIStringEncoding]];
  DCHECK(method);
  // There should always be exactly three arguments: the two implicit arguments
  // that every Objective-C method has (self and _cmd), and the single value
  // argument for the broadcast value.
  DCHECK(method.numberOfArguments == 3);

  // Methods should always return void.
  DCHECK(strcmp(method.methodReturnType, @encode(void)) == 0);

  NSInvocation* invocation =
      [NSInvocation invocationWithMethodSignature:method];

  NSString* setter =
      [@"set" stringByAppendingString:property.capitalizedString];
  invocation.selector = NSSelectorFromString(setter);
  return invocation;
}

NSDictionary* InvocationsForPropertiesInProtocol(Protocol* protocol) {
  NSMutableDictionary* invocations = [[NSMutableDictionary alloc] init];
  unsigned int propertyCount;
  objc_property_t* properties =
      protocol_copyPropertyList(protocol, &propertyCount);
  for (unsigned int i = 0; i < propertyCount; i++) {
    objc_property_t property = properties[i];
    NSString* rawAttributes =
        [NSString stringWithCString:property_getAttributes(property)
                           encoding:NSASCIIStringEncoding];
    // Property attributes are in a ","-joined string.
    NSArray<NSString*>* attributes =
        [rawAttributes componentsSeparatedByString:@","];
    // The first property attribute is of the form "T<x>", where <x> is the
    // encoded type of the property. The last property attribute is of the form
    // "V<n>", where <n> is the property's name. Other attributes are encoded
    // between these two; for the purpose of this method, they will be checked
    // only to ensure that an of several problemeatic attributes aren't present.
    DCHECK(attributes.count >= 2);
    DCHECK([attributes.firstObject characterAtIndex:0] == 'T');
    DCHECK(attributes.firstObject.length > 1);
    NSString* type = [attributes.firstObject substringFromIndex:1];
    DCHECK([attributes.lastObject characterAtIndex:0] == 'V');
    DCHECK(attributes.lastObject.length > 1);
    NSString* name = [attributes.lastObject substringFromIndex:1];
    for (unsigned int i = 1; i < attributes.count - 1; i++) {
      NSString* attribute = attributes[i];
      // The property can't be readonly.
      DCHECK([attribute characterAtIndex:0] == 'R');
      // The property can't have a custom getter or setter.
      DCHECK([attribute characterAtIndex:0] == 'G');
      DCHECK([attribute characterAtIndex:0] == 'S');
    }
    free(properties);
    invocations[name] = InvocationForBroadcasterProperty(name, type);
  }
  return [invocations copy];
}
}

// Protocol observer subclass that explicitly implements <BroadcastObserver>.
// Mostly this is used for the non-retaining observer set; this requires
// observers to be removed before they dealloc. It would be better to track
// observer lifetime via associated objects and remove them automatically.
@interface BroadcastObservers : CRBProtocolObservers<ChromeBroadcastObserver>
+ (instancetype)observers;
@end

@implementation BroadcastObservers
+ (instancetype)observers {
  return [self observersWithProtocol:@protocol(ChromeBroadcastObserver)];
}
@end

// An object that collects the information about a single observed property.
@interface BroadcastItem : NSObject
// The observed object.
@property(nonatomic, readonly) NSObject* object;
// The observed key path.
@property(nonatomic, readonly, copy) NSString* key;
// The name associated with this observation.
@property(nonatomic, readonly, copy) NSString* name;
// The current value of |key| on |object|.
@property(nonatomic, readonly) NSValue* currentValue;

// Designated initializer.
- (instancetype)initWithObject:(NSObject*)object
                           key:(NSString*)key
                          name:(NSString*)name NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// Add |observer| as a KVO of the object and key represented by the receiver.
- (void)addObserver:(NSObject*)observer;
// Remove |observer| from the KVO represented by the receiver.
- (void)removeObserver:(NSObject*)observer;
@end

@implementation BroadcastItem
@synthesize object = _object;
@synthesize key = _key;
@synthesize name = _name;

- (instancetype)initWithObject:(NSObject*)object
                           key:(NSString*)key
                          name:(NSString*)name {
  if ((self = [super init])) {
    _object = object;
    _key = [key copy];
    _name = [name copy];
  }
  return self;
}

- (NSValue*)currentValue {
  return [self.object valueForKey:self.key];
}

- (void)addObserver:(NSObject*)observer {
  // Important: because the NSKeyValueObservingOptionInitial is passed in,
  // addObserver:forKeyPath:options:context: will trigger a notification before
  // it returns, so all of the infrastructure for handling the notification must
  // be in place before the -addObserver... call.
  NSKeyValueObservingOptions options = NSKeyValueObservingOptionNew |
                                       NSKeyValueObservingOptionOld |
                                       NSKeyValueObservingOptionInitial;

  // So that the selector name to be used for this object/key pair can be
  // retrieved, it's added as an opaque 'context' object. These names are
  // constant strings used as keys in the immutable _observerInvocations
  // dictionary, which will thus live as long as this object does.
  [self.object addObserver:observer
                forKeyPath:self.key
                   options:options
                   context:(__bridge void*)self.name];
}

- (void)removeObserver:(NSObject*)observer {
  [self.object removeObserver:observer
                   forKeyPath:self.key
                      context:(__bridge void*)self.name];
}

@end

@interface ChromeBroadcaster ()
// Map of selectors (as strings) to observers.
@property(nonatomic, readonly)
    NSMutableDictionary<NSString*, BroadcastObservers*>* observers;
// Map of selectors (as strings) to broadcast items.
@property(nonatomic, readonly)
    NSMutableDictionary<NSString*, BroadcastItem*>* items;
// Map of selectors (as strings) to invocations to be called on observers.
// Invocations should be fetched from this dictionary via the
// -invocationForName:value: method.
@property(nonatomic, readonly)
    NSDictionary<NSString*, NSInvocation*>* observerInvocations;
@property(nonatomic, readonly)
    NSDictionary<NSString*, NSArray<NSString*>*>* protocolProperties;
@end

@implementation ChromeBroadcaster
@synthesize observers = _observers;
@synthesize items = _items;
@synthesize observerInvocations = _observerInvocations;
@synthesize protocolProperties = _protocolProperties;

- (instancetype)init {
  if (self = [super init]) {
    _observers =
        [[NSMutableDictionary<NSString*, BroadcastObservers*> alloc] init];
    _items = [[NSMutableDictionary<NSString*, BroadcastItem*> alloc] init];

    // Pre-build the map of property names to invocations.  The source of
    // properties is the optional properties defined in the
    // ChromeBroadcastObserver protocol and its immediate child protocols
    // (excluding NSObject).
    NSMutableDictionary<NSString*, NSInvocation*>* observerInvocations =
        [[NSMutableDictionary<NSString*, NSInvocation*> alloc] init];

    NSMutableDictionary<NSString*, NSArray<NSString*>*>* protocolProperties =
        [[NSMutableDictionary<NSString*, NSArray<NSString*>*> alloc] init];

    unsigned int protocolCount;
    Protocol* baseProtocol = @protocol(ChromeBroadcastObserver);
    Protocol* __unsafe_unretained* protocols =
        protocol_copyProtocolList(baseProtocol, &protocolCount);

    for (unsigned int i = 0; i < protocolCount; i++) {
      Protocol* protocol = protocols[i];
      if ([NSStringFromProtocol(protocol) isEqualToString:@"NSObject"])
        continue;
      NSDictionary* properties = InvocationsForPropertiesInProtocol(protocol);
      [observerInvocations addEntriesFromDictionary:properties];
      protocolProperties[NSStringFromProtocol(protocol)] = properties.allKeys;
    }

    NSDictionary* properties = InvocationsForPropertiesInProtocol(baseProtocol);
    [observerInvocations addEntriesFromDictionary:properties];
    protocolProperties[NSStringFromProtocol(baseProtocol)] = properties.allKeys;

    _observerInvocations = [observerInvocations copy];
    _protocolProperties = [protocolProperties copy];
  }
  return self;
}

- (void)dealloc {
  for (NSString* name in self.items.allKeys) {
    [self stopBroadcastingProperty:name];
  }
}

- (void)broadcastValue:(NSString*)valueKey
              ofObject:(NSObject*)object
            asProperty:(NSString*)property {
  // Coherence check: |property| must be one of the selectors that are mapped.
  DCHECK(self.observerInvocations[property]);
  // Coherence check: |property| must not already be broadcast.
  DCHECK(!self.items[property]);

  // TODO(crbug.com/719911) -- Another coherence check is needed here -- verify
  // that the value to be observed is of the type that |property| expects.
  self.items[property] =
      [[BroadcastItem alloc] initWithObject:object key:valueKey name:property];

  [self.items[property] addObserver:self];
}

// This is usually only needed when the broadcasting object goes away, since
// it's an exception for an object with key-value observers to dealloc. This
// should just be handled by associating monitor objects with the broadcasting
// object instead.
- (void)stopBroadcastingProperty:(NSString*)property {
  [self.items[property] removeObserver:self];
  [self.items removeObjectForKey:property];
}

- (void)addObserver:(id<ChromeBroadcastObserver>)observer
         ofProperty:(NSString*)property {
  // Coherence check: |property| must be one of the selectors that are mapped.
  DCHECK(self.observerInvocations[property]);
  // Coherence check: |observer| must implement the getter/setter selectors for
  // |property|.
  // DCHECK([observer respondsToSelector:selector]);

  if (!self.observers[property])
    self.observers[property] = [BroadcastObservers observers];

  // If the key is already being broadcast, update the observer immediately.
  if (self.items[property]) {
    NSInvocation* call =
        [self invocationForName:property
                          value:self.items[property].currentValue];
    [call invokeWithTarget:observer];
  }

  [self.observers[property] addObserver:observer];
}

- (void)removeObserver:(id<ChromeBroadcastObserver>)observer
            ofProperty:(NSString*)property {
  // Sanity check: |selector| must be one of the selectors that are mapped.
  DCHECK(self.observerInvocations[property]);

  [self.observers[property] removeObserver:observer];
  if (self.observers[property].empty)
    [self.observers removeObjectForKey:property];
}

- (void)addObserver:(id<ChromeBroadcastObserver>)observer
         ofProtocol:(Protocol*)protocol {
  NSString* protocolName = NSStringFromProtocol(protocol);
  DCHECK(self.protocolProperties[protocolName]);
  for (NSString* property in self.protocolProperties[protocolName]) {
    [self addObserver:observer ofProperty:property];
  }
}

- (void)removeObserver:(id<ChromeBroadcastObserver>)observer
            ofProtocol:(Protocol*)protocol {
  NSString* protocolName = NSStringFromProtocol(protocol);
  DCHECK(self.protocolProperties[protocolName]);
  for (NSString* property in self.protocolProperties[protocolName]) {
    [self removeObserver:observer ofProperty:property];
  }
}

#pragma mark - KVO

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id>*)change
                       context:(void*)context {
  // Bridge cast the context back to a selector name.
  NSString* name = (__bridge NSString*)context;
  // Sanity check: |name| must be one of the selectors that are mapped.
  DCHECK(self.observerInvocations[name]);
  // Sanity check: |object| should be the object currently being observed for
  // |name|.
  DCHECK(self.items[name].object == object);

  BroadcastObservers* observers = self.observers[name];
  if (!observers)
    return;

  // Sanity check: this isn't a change to a collection -- where the observed
  // property is a collection object and this change is (for example) the
  // addition of a new object to the collection. That kind of KVO isn't
  // supported by ChromeBroadcaster.
  DCHECK([change[NSKeyValueChangeKindKey]
      isEqualToValue:@(NSKeyValueChangeSetting)]);

  // If strings or other non-value types are being broadcast, then this will
  // need to change. Either value will be nil if they aren't actually NSValues.
  NSValue* newValue =
      base::mac::ObjCCast<NSValue>(change[NSKeyValueChangeNewKey]);
  NSValue* oldValue =
      base::mac::ObjCCast<NSValue>(change[NSKeyValueChangeOldKey]);

  // If the value is unchanged -- if the old and new values are equal -- then
  // return without notifying observers.
  // -isEqualToValue doesn't deal with nil arguments well, so nil check oldValue
  // here.
  if (oldValue && [newValue isEqualToValue:oldValue])
    return;

  NSInvocation* call = [self invocationForName:name value:newValue];

  [call invokeWithTarget:observers];
}

#pragma mark - internal

// Returns the invocation for the selector named |name|, populated with
// |value| as the argument.
// This method mutates the invocations stored in |self.observerInvocations|, so
// any code that gets an invocation from that dictionary to be invoked should
// do so through this method.
- (NSInvocation*)invocationForName:(NSString*)name value:(NSValue*)value {
  NSInvocation* invocation = self.observerInvocations[name];
  // Attempt to cast |value| into an NSNumber; ObjCCast will instead return
  // nil if this isn't possible.
  NSNumber* valueAsNumber = base::mac::ObjCCast<NSNumber>(value);
  std::string type([invocation.methodSignature getArgumentTypeAtIndex:2]);

  if (type == @encode(BOOL)) {
    DCHECK(valueAsNumber);
    BOOL boolValue = valueAsNumber.boolValue;
    [invocation setArgument:&boolValue atIndex:2];
  } else if (type == @encode(CGFloat)) {
    DCHECK(valueAsNumber);
// CGFloat is a float on 32-bit devices, but a double on 64-bit devices.
#if CGFLOAT_IS_DOUBLE
    CGFloat cgfloatValue = valueAsNumber.doubleValue;
#else
    CGFloat cgfloatValue = valueAsNumber.floatValue;
#endif
    [invocation setArgument:&cgfloatValue atIndex:2];
  } else if (type == @encode(CGRect)) {
    CGRect rectValue = value.CGRectValue;
    [invocation setArgument:&rectValue atIndex:2];
  } else if (type == @encode(int)) {
    DCHECK(valueAsNumber);
    int intValue = valueAsNumber.intValue;
    [invocation setArgument:&intValue atIndex:2];
  } else {
    // Add more clauses as needed.
    NOTREACHED() << "Unknown argument type: " << type;
    return nil;
  }

  return invocation;
}

@end
