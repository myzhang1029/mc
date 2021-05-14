#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif
#include <stdio.h>
#include <string.h>

#include "tool.h"

#define CHECK_ERROR(msg)                                                       \
    do                                                                         \
    {                                                                          \
        if (stat != CL_SUCCESS)                                                \
        {                                                                      \
            fprintf(stderr, (msg));                                            \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

int main(void)
{
    const double r = 5.0f;
    unsigned long rand_samples = 1280000000ul;
    unsigned long each;
    unsigned long inside = 12;
    cl_platform_id *plats;
    cl_device_id **devices;
    cl_device_id to_use;
    cl_context ctx;
    cl_command_queue cq;
    cl_program program;
    cl_mem outmem;
    cl_kernel kernel;
    cl_int stat;
    char *prog;
    size_t proglen[1];
    bool *results;
    unsigned long wgsize;
    const char *filename = "mc_hypot_opencl.cl";

    plats = get_platforms();
    if (!plats)
        return 2;
    devices = get_devices(plats[0]);
    free(plats);
    if (devices[1])
        to_use = devices[1][0];
    else if (devices[0])
        to_use = devices[0][0];
    else
        return 2;
    ctx = clCreateContext(NULL, 1, &to_use, NULL, NULL, &stat);
    CHECK_ERROR("Error creating context\n");
    cq = clCreateCommandQueue(ctx, to_use, 0, &stat);
    CHECK_ERROR("Error creating command queue\n");

    /* Load program */
    prog = file2str(filename);
    proglen[0] = strlen(prog);

    program =
        clCreateProgramWithSource(ctx, 1, (const char **)&prog, proglen, &stat);
    stat |= clBuildProgram(program, 1, &to_use, "-Werror", NULL, NULL);
    if (stat != CL_SUCCESS)
    {
        size_t len;
        char buffer[2048];
        fprintf(stderr, "Error building program\n");
        clGetProgramBuildInfo(program, to_use, CL_PROGRAM_BUILD_LOG,
                              sizeof(buffer), buffer, &len);
        fprintf(stderr, "%s\n", buffer);
        exit(1);
    }

    kernel = clCreateKernel(program, "monte_carlo", &stat);
    CHECK_ERROR("Error creating kernel\n");

    stat = clGetKernelWorkGroupInfo(kernel, to_use, CL_KERNEL_WORK_GROUP_SIZE,
                                    sizeof(wgsize), &wgsize, NULL);
    CHECK_ERROR("Error retrieving kernel work group info\n");

    results = malloc(sizeof(bool) * rand_samples);

    /* Prepare parameters */
    outmem = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, sizeof(bool) * rand_samples,
                            NULL, &stat);
    CHECK_ERROR("Error creating buffer\n");

    /* radius */
    stat = clSetKernelArg(kernel, 0, sizeof(double), (void *)&r);
    /* The output list */
    stat |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&outmem);
    /* Size of outmem as modulo */
    stat |=
        clSetKernelArg(kernel, 2, sizeof(unsigned int), (void *)&rand_samples);
    CHECK_ERROR("Error setting args\n");
    /* Issue calls */
    stat = clEnqueueNDRangeKernel(cq, kernel, 1, NULL, &rand_samples, &wgsize,
                                  0, NULL, NULL);
    CHECK_ERROR("Error executing kernel\n");
    /* Wait all */
    clFinish(cq);
    stat =
        clEnqueueReadBuffer(cq, outmem, CL_TRUE, 0, sizeof(bool) * rand_samples,
                            results, 0, NULL, NULL);
    CHECK_ERROR("Error reading output array\n");
    for (size_t i = 0; i < rand_samples; ++i)
    {
        inside += results[i];
    }

    printf("%lu/%lu\n", inside, rand_samples);
    printf("%g\n", inside / (double)rand_samples * 4 * r * r);

    /* Clean up */
    free(results);
    stat = clReleaseMemObject(outmem);
    stat |= clReleaseKernel(kernel);
    stat |= clReleaseProgram(program);
    free(prog);
    stat |= clReleaseCommandQueue(cq);
    stat |= clReleaseContext(ctx);
    free_devices(devices);
    return 0;
}
