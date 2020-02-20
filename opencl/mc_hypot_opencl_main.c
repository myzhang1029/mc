#ifdef KERNEL_PROGRAM
#else
#include <OpenCL/opencl.h>
#include <stdio.h>
#include <string.h>

#include "tool.h"

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
    size_t nprocs;
    size_t i;
    unsigned long lnprocs;
    unsigned long *results;
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
    ctx = clCreateContext(NULL, 1, &to_use, NULL, NULL, NULL);
    cq = clCreateCommandQueue(ctx, to_use, 0, NULL);

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
        clGetProgramBuildInfo(program, to_use, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        fprintf(stderr, "%s\n", buffer);
        exit(1);
    }
    
    kernel = clCreateKernel(program, "monte_carlo", &stat);
    if (stat != CL_SUCCESS)
    {
        fprintf(stderr, "Error creating kernel\n");
        exit(1);
    }
    
    stat = clGetKernelWorkGroupInfo(kernel, to_use, CL_KERNEL_WORK_GROUP_SIZE, sizeof(nprocs), &nprocs, NULL);
    lnprocs = (unsigned long) nprocs;
    if (stat != CL_SUCCESS)
    {
        fprintf(stderr, "Error retrieving kernel work group info\n");
        exit(1);
    }
    results = malloc(sizeof(unsigned long) * nprocs);
    
    /* Prepare parameters */
    outmem = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, sizeof(unsigned long) * nprocs, NULL, &stat);
    if (stat != CL_SUCCESS)
    {
        fprintf(stderr, "Error creating buffer\n");
        exit(1);
    }

    each = rand_samples / nprocs;
    if (rand_samples % nprocs != 0)
    {
        fprintf(stderr, "Warning: work adjusted: %lu%%%lu\n", rand_samples, nprocs);
        rand_samples = nprocs * each;
    }

    /* radius */
    stat = clSetKernelArg(kernel, 0, sizeof(double), (void *)&r);
    /* rand_samples for each proc */
    stat |= clSetKernelArg(kernel, 1, sizeof(unsigned long), (void *)&each);
    /* The output list */
    stat |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&outmem);
    /* procs in total */
    stat |= clSetKernelArg(kernel, 3, sizeof(unsigned long), (void *)&lnprocs);
    /* Something random to be used as initseq */
    stat |= clSetKernelArg(kernel, 4, sizeof(unsigned long), (void *)&results);
    if (stat != CL_SUCCESS)
    {
        fprintf(stderr, "Error setting args\n");
        exit(1);
    }
    /* Issue calls */
    stat = clEnqueueNDRangeKernel(cq, kernel, 1, NULL, &lnprocs, &lnprocs, 0, NULL, NULL);
    if (stat != CL_SUCCESS)
    {
        fprintf(stderr, "Failed to execute kernel!\n");
        exit(1);
    }
    /* Wait all */
    clFinish(cq);
    stat = clEnqueueReadBuffer(cq, outmem, CL_TRUE, 0, sizeof(unsigned long) * nprocs, results, 0, NULL, NULL);  
    if (stat != CL_SUCCESS)
    {
        fprintf(stderr, "Error reading output array\n");
        exit(1);
    }
    for (i = 0; i < nprocs; ++i)
    {
        inside += results[i];
    }

    printf("%lu/%lu\n", inside, rand_samples);
    printf("%g\n", inside / (double) rand_samples * 4 * r * r);

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
#endif
