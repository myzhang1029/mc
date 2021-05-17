/*
See LICENSE folder for this sample’s licensing information.

Abstract:
An app that performs a simple calculation on a GPU.
*/

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import "MetalMonteCarlo.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();

        // Create the custom object used to encapsulate the Metal code.
        // Initializes objects to communicate with the GPU.
        MetalMonteCarlo* mc = [[MetalMonteCarlo alloc] initDevice:device];
        
        // Create buffers to hold results
        [mc allocResult];
        
        // Send a command to the GPU to perform the calculation.
        [mc startCompute];

        NSLog(@"Execution finished");
    }
    return 0;
}
