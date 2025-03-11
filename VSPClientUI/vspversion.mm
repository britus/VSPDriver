#import <Foundation/Foundation.h>

extern "C" {
const char* GetBundleVersion() {
    NSBundle *bundle = [NSBundle mainBundle];
    if (!bundle) return "Unknown";

    NSDictionary *infoDict = [bundle infoDictionary];
    NSString *version = [infoDict objectForKey:@"CFBundleShortVersionString"];
    return version ? [version UTF8String] : "0.0";
}

const char* GetBuildNumber() {
    NSBundle *bundle = [NSBundle mainBundle];
    if (!bundle) return "Unknown";

    NSDictionary *infoDict = [bundle infoDictionary];
    NSString *build = [infoDict objectForKey:@"CFBundleVersion"];
    return build ? [build UTF8String] : "0";
}
}
