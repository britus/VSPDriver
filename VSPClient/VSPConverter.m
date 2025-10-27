//
//  VSPConverter.m
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#import <Foundation/Foundation.h>
#import "VSPConverter.h"

#import "VSPDriverDataModel.h"

@implementation TVSPConverter

+ (TVSPControllerData *)convertFromCPP:(const void *)cppPointer size:(size_t)size {
    if (!cppPointer || size < sizeof(CVSPDriverData)) {
        return nil;
    }

    const CVSPDriverData *cppData = (const CVSPDriverData *)cppPointer;

    // Status
    TVSPControllerStatus *status = [[TVSPControllerStatus alloc] initWithCode:cppData->status.code
                                                                        flags:cppData->status.flags];

    // Parameter + PortLink
    TVSPPortLink *link = [[TVSPPortLink alloc] initWithSource:cppData->parameter.link.source
                                                       target:cppData->parameter.link.target];
    TVSPControllerParameter *parameter = [[TVSPControllerParameter alloc] initWithFlags:cppData->parameter.flags
                                                                                  link:link];

    // PortList
    NSMutableArray<TVSPPortListItem *> *portItems = [NSMutableArray array];
    for (uint8_t i = 0; i < cppData->ports.count; i++) {
        char *nameCStr = (char *)cppData->ports.list[i].name;
        NSString *name = nameCStr ? [NSString stringWithUTF8String:nameCStr] : @"";
        TVSPPortListItem *item = [[TVSPPortListItem alloc] initWithPortId:cppData->ports.list[i].id
                                                                    flags:cppData->ports.list[i].flags
                                                                     name:name];
        [portItems addObject:item];
    }
    TVSPPortList *ports = [[TVSPPortList alloc] initWithCount:cppData->ports.count
                                                        items:portItems];

    // LinkList
    NSMutableArray<NSNumber *> *linkNumbers = [NSMutableArray array];
    for (uint8_t i = 0; i < cppData->links.count; i++) {
        [linkNumbers addObject:@(cppData->links.list[i])];
    }
    TVSPLinkList *links = [[TVSPLinkList alloc] initWithCount:cppData->links.count
                                                       links:linkNumbers];

    // Final ControllerData
    TVSPControllerData *objcData = [[TVSPControllerData alloc] initWithContext:cppData->context
                                                                       command:cppData->command
                                                                        status:status
                                                                     parameter:parameter
                                                                         ports:ports
                                                                         links:links];
    return objcData;
}

@end
