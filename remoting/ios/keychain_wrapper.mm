// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/keychain_wrapper.h"

#include "base/logging.h"
#include "base/mac/scoped_cftyperef.h"

#import "remoting/ios/domain/host_info.h"

static const UInt8 kKeychainItemIdentifier[] = "org.chromium.RemoteDesktop\0";

NSString* const kPairingSecretSeperator = @"|";

NSString* const kKeychainPairingId = @"kKeychainPairingId";
NSString* const kKeychainPairingSecret = @"kKeychainPairingSecret";

@interface KeychainWrapper () {
  NSMutableDictionary* _keychainData;
  NSMutableDictionary* _userInfoQuery;
}
@end

@implementation KeychainWrapper

// KeychainWrapper is a singleton.
+ (KeychainWrapper*)instance {
  static KeychainWrapper* sharedInstance = nil;
  static dispatch_once_t guard;
  dispatch_once(&guard, ^{
    sharedInstance = [[KeychainWrapper alloc] init];
  });
  return sharedInstance;
}

- (id)init {
  if ((self = [super init])) {
    OSStatus keychainErr = noErr;
    _userInfoQuery = [[NSMutableDictionary alloc] init];
    _userInfoQuery[(__bridge id)kSecClass] =
        (__bridge id)kSecClassGenericPassword;
    NSData* keychainItemID =
        [NSData dataWithBytes:kKeychainItemIdentifier
                       length:strlen((const char*)kKeychainItemIdentifier)];
    _userInfoQuery[(__bridge id)kSecAttrService] = keychainItemID;
    _userInfoQuery[(__bridge id)kSecMatchLimit] =
        (__bridge id)kSecMatchLimitOne;
    _userInfoQuery[(__bridge id)kSecReturnAttributes] =
        (__bridge id)kCFBooleanTrue;

    base::ScopedCFTypeRef<CFMutableDictionaryRef> outDictionary;
    keychainErr =
        SecItemCopyMatching((__bridge CFDictionaryRef)_userInfoQuery,
                            (CFTypeRef*)outDictionary.InitializeInto());
    if (keychainErr == noErr) {
      _keychainData = [self
          secItemFormatToDictionary:(__bridge_transfer NSMutableDictionary*)
                                        outDictionary.release()];
    } else if (keychainErr == errSecItemNotFound) {
      [self resetKeychainItem];

      if (outDictionary) {
        _keychainData = nil;
      }
    } else {
      LOG(FATAL) << "Serious error: " << keychainErr;
      if (outDictionary) {
        _keychainData = nil;
      }
    }
  }
  return self;
}

#pragma mark - Public

- (void)setRefreshToken:(NSString*)refreshToken {
  [self setObject:refreshToken forKey:(__bridge id)kSecValueData];
}

- (NSString*)refreshToken {
  return [self objectForKey:(__bridge id)kSecValueData];
}

- (void)commitPairingCredentialsForHost:(NSString*)host
                                     id:(NSString*)pairingId
                                 secret:(NSString*)secret {
  NSString* keysString = [self objectForKey:(__bridge id)kSecAttrGeneric];
  NSMutableDictionary* keys = [self stringToMap:keysString];
  NSString* pairingIdAndSecret = [NSString
      stringWithFormat:@"%@%@%@", pairingId, kPairingSecretSeperator, secret];
  keys[host] = pairingIdAndSecret;
  [self setObject:[self mapToString:keys] forKey:(__bridge id)kSecAttrGeneric];
}

- (NSDictionary*)pairingCredentialsForHost:(NSString*)host {
  NSString* keysString = [self objectForKey:(__bridge id)kSecAttrGeneric];
  NSMutableDictionary* keys = [self stringToMap:keysString];
  NSString* pairingIdAndSecret = keys[host];
  if (!pairingIdAndSecret ||
      [pairingIdAndSecret rangeOfString:kPairingSecretSeperator].location ==
          NSNotFound) {
    return nil;
  }
  NSArray* components =
      [pairingIdAndSecret componentsSeparatedByString:kPairingSecretSeperator];
  DCHECK(components.count == 2);
  return @{
    kKeychainPairingId : components[0],
    kKeychainPairingSecret : components[1],
  };
}

#pragma mark - Map to String helpers

- (NSMutableDictionary*)stringToMap:(NSString*)mapString {
  NSError* err;

  if (mapString &&
      [mapString respondsToSelector:@selector(dataUsingEncoding:)]) {
    NSData* data = [mapString dataUsingEncoding:NSUTF8StringEncoding];
    NSDictionary* pairingMap =
        data ? (NSDictionary*)[NSJSONSerialization
                   JSONObjectWithData:data
                              options:NSJSONReadingMutableContainers
                                error:&err]
             : @{};
    if (!err) {
      return [NSMutableDictionary dictionaryWithDictionary:pairingMap];
    }
  }
  // failed to load a dictionary, make a new one.
  return [NSMutableDictionary dictionaryWithCapacity:1];
}

- (NSString*)mapToString:(NSDictionary*)map {
  if (map) {
    NSError* err;
    NSData* jsonData =
        [NSJSONSerialization dataWithJSONObject:map options:0 error:&err];
    if (!err) {
      return [[NSString alloc] initWithData:jsonData
                                   encoding:NSUTF8StringEncoding];
    }
  }
  // failed to convert the map, make nil string.
  return nil;
}

#pragma mark - Private

// Implement the mySetObject:forKey method, which writes attributes to the
// keychain:
- (void)setObject:(id)inObject forKey:(id)key {
  if (inObject == nil)
    return;
  id currentObject = _keychainData[key];
  if (![currentObject isEqual:inObject]) {
    _keychainData[key] = inObject;
    [self writeToKeychain];
  }
}

// Implement the myObjectForKey: method, which reads an attribute value from a
// dictionary:
- (id)objectForKey:(id)key {
  return _keychainData[key];
}

- (void)resetKeychainItem {
  if (!_keychainData) {
    _keychainData = [[NSMutableDictionary alloc] init];
  } else if (_keychainData) {
    NSMutableDictionary* tmpDictionary =
        [self dictionaryToSecItemFormat:_keychainData];
    OSStatus errorcode = SecItemDelete((__bridge CFDictionaryRef)tmpDictionary);
    if (errorcode == errSecItemNotFound) {
      // this is ok.
    } else if (errorcode != noErr) {
      LOG(FATAL) << "Problem deleting current keychain item. errorcode: "
                 << errorcode;
    }
  }

  _keychainData[(__bridge id)kSecAttrLabel] = @"gaia_refresh_token";
  _keychainData[(__bridge id)kSecAttrDescription] = @"Gaia fresh token";
  _keychainData[(__bridge id)kSecValueData] = @"";
  _keychainData[(__bridge id)kSecClass] = @"";
  _keychainData[(__bridge id)kSecAttrGeneric] = @"";
}

- (NSMutableDictionary*)dictionaryToSecItemFormat:
    (NSDictionary*)dictionaryToConvert {
  NSMutableDictionary* returnDictionary =
      [NSMutableDictionary dictionaryWithDictionary:dictionaryToConvert];

  NSData* keychainItemID =
      [NSData dataWithBytes:kKeychainItemIdentifier
                     length:strlen((const char*)kKeychainItemIdentifier)];
  returnDictionary[(__bridge id)kSecAttrService] = keychainItemID;
  returnDictionary[(__bridge id)kSecClass] =
      (__bridge id)kSecClassGenericPassword;

  NSString* passwordString = dictionaryToConvert[(__bridge id)kSecValueData];
  returnDictionary[(__bridge id)kSecValueData] =
      [passwordString dataUsingEncoding:NSUTF8StringEncoding];
  return returnDictionary;
}

- (NSMutableDictionary*)secItemFormatToDictionary:
    (NSDictionary*)dictionaryToConvert {
  NSMutableDictionary* returnDictionary =
      [NSMutableDictionary dictionaryWithDictionary:dictionaryToConvert];

  returnDictionary[(__bridge id)kSecReturnData] = (__bridge id)kCFBooleanTrue;
  returnDictionary[(__bridge id)kSecClass] =
      (__bridge id)kSecClassGenericPassword;

  base::ScopedCFTypeRef<CFDataRef> passwordData;
  OSStatus keychainError = noErr;
  keychainError =
      SecItemCopyMatching((__bridge CFDictionaryRef)returnDictionary,
                          (CFTypeRef*)passwordData.InitializeInto());
  if (keychainError == noErr) {
    [returnDictionary removeObjectForKey:(__bridge id)kSecReturnData];

    NSString* password =
        [[NSString alloc] initWithBytes:CFDataGetBytePtr(passwordData)
                                 length:CFDataGetLength(passwordData)
                               encoding:NSUTF8StringEncoding];
    returnDictionary[(__bridge id)kSecValueData] = password;
  } else if (keychainError == errSecItemNotFound) {
    LOG(WARNING) << "Nothing was found in the keychain.";
  } else {
    LOG(FATAL) << "Serious error: " << keychainError;
  }
  return returnDictionary;
}

- (void)writeToKeychain {
  base::ScopedCFTypeRef<CFDictionaryRef> attributes;
  NSMutableDictionary* updateItem = nil;

  if (SecItemCopyMatching((__bridge CFDictionaryRef)_userInfoQuery,
                          (CFTypeRef*)attributes.InitializeInto()) == noErr) {
    updateItem = [NSMutableDictionary
        dictionaryWithDictionary:(__bridge_transfer NSDictionary*)
                                     attributes.release()];

    updateItem[(__bridge id)kSecClass] = _userInfoQuery[(__bridge id)kSecClass];

    NSMutableDictionary* tempCheck =
        [self dictionaryToSecItemFormat:_keychainData];
    [tempCheck removeObjectForKey:(__bridge id)kSecClass];

    OSStatus errorcode = SecItemUpdate((__bridge CFDictionaryRef)updateItem,
                                       (__bridge CFDictionaryRef)tempCheck);
    if (errorcode != noErr) {
      LOG(FATAL) << "Couldn't update the Keychain Item. errorcode: "
                 << errorcode;
    }
  } else {
    OSStatus errorcode =
        SecItemAdd((__bridge CFDictionaryRef)
                       [self dictionaryToSecItemFormat:_keychainData],
                   NULL);
    if (errorcode != noErr) {
      LOG(FATAL) << "Couldn't add the Keychain Item. errorcode: " << errorcode;
    }
  }
}

@end
