/*
See LICENSE folder for this sample’s licensing information.

Abstract:
A class to manage all of the Metal objects this app creates.
*/

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

NS_ASSUME_NONNULL_BEGIN

@interface MetalMonteCarlo : NSObject
- (instancetype) initDevice: (id<MTLDevice>) device;
- (void) allocResult;
- (void) startCompute;
@end

NS_ASSUME_NONNULL_END
