// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#ifndef VSPFramework_h
#define VSPFramework_h

#import <Foundation/Foundation.h>

//! Project version number for TVSPFramework.
FOUNDATION_EXPORT double TVSPFrameworkVersionNumber;

//! Project version string for TVSPFramework.
FOUNDATION_EXPORT const unsigned char TVSPFrameworkVersionString[];

// In this header, you should import all the public headers of your
// framework using statements like #import <VSPFramework/PublicHeader.h>

#import "TVSPUserContext.h"
#import "TVSPControlCommand.h"
#import "TVSPPortListItem.h"
#import "TVSPControllerStatus.h"
#import "TVSPPortLink.h"
#import "TVSPControllerParameter.h"
#import "TVSPPortList.h"
#import "TVSPLinkList.h"
#import "TVSPControllerData.h"
#import "TVSPPortParameters.h"

#endif /* VSPFramework_h */
