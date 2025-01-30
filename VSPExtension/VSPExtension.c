//
//  VSPExtension.c
//  VSPExtension
//
//  Created by Björn Eschrich on 30.01.25.
//

#include <mach/mach_types.h>

kern_return_t VSPExtension_start(kmod_info_t * ki, void *d);
kern_return_t VSPExtension_stop(kmod_info_t *ki, void *d);

kern_return_t VSPExtension_start(kmod_info_t * ki, void *d)
{
    return KERN_SUCCESS;
}

kern_return_t VSPExtension_stop(kmod_info_t *ki, void *d)
{
    return KERN_SUCCESS;
}
