/* OpenCL helper program for macOS OpenCL */
#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "tool.h"

static void *xmalloc(size_t size)
{
    void *buf = malloc(size);
    if (buf == NULL)
    {
        fprintf(stderr, "Memory failure\n");
        abort();
    }
    return buf;
}

char *file2str(const char *restrict fn)
{
    FILE *fp = fopen(fn, "rb");
    long filesize;
    char *things;
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    things = xmalloc((1 + filesize) * sizeof(char));
    fseek(fp, 0, SEEK_SET);
    fread(things, 1, filesize, fp);
    things[filesize] = 0;
    return things;
}

int test_file2str(int argc, char **argv)
{
    char *str;
    if (argc != 2)
        return fprintf(stderr, "Usage: test file\n");
    str = file2str(argv[1]);
    puts(str);
    free(str);
    return 0;
}

int print_platform_info(cl_platform_id platform)
{
    cl_int stat;
    size_t ntypes = 1, i;
    size_t paramsize;
    void *buf;
    cl_platform_info infotypes[] = {
        CL_PLATFORM_NAME,
    };
    for (i = 0; i < ntypes; ++i)
    {
        stat = 0;
        stat = clGetPlatformInfo(platform, infotypes[i], 0, NULL, &paramsize);
        buf = xmalloc(paramsize * sizeof(char));
        stat |= clGetPlatformInfo(platform, infotypes[i], paramsize, buf, NULL);
        if (stat != CL_SUCCESS)
            return -1;
        printf("%s\n", (char *)buf);
        free(buf);
    }
    return 0;
}

/* Get OpenCL platforms */
cl_platform_id *get_platforms(void)
{
    cl_uint num;
    cl_uint i;
    /* Get the number of devices */
    cl_int stat = clGetPlatformIDs(0, NULL, &num);
    if (stat != CL_SUCCESS)
    {
        fprintf(stderr, "Error getting platforms!\n");
        abort();
    }
    if (num > 0)
    {
        cl_platform_id *platforms =
            (cl_platform_id *)xmalloc((num + 1) * sizeof(cl_platform_id));
        stat = clGetPlatformIDs(num, platforms, NULL);
        printf("[INFO]: Got %d platform(s)\n", num);
        for (i = 0; i < num; ++i)
        {
            printf("[INFO]: Platform %d's info:\n", i + 1);
            print_platform_info(platforms[i]);
        }
        platforms[num] = NULL;
        return platforms;
    }
    else
        /* No platforms */
        return NULL;
}

int test_platforms(void)
{
    cl_platform_id *platforms = get_platforms();
    free(platforms);
    return 0;
}

int print_device_info(cl_device_id device)
{
    cl_int stat;
    size_t ntypes = 1, i;
    size_t paramsize;
    void *buf;
    cl_device_info infotypes[] = {
        CL_DEVICE_NAME,
    };
    for (i = 0; i < ntypes; ++i)
    {
        stat = 0;
        stat = clGetDeviceInfo(device, infotypes[i], 0, NULL, &paramsize);
        buf = xmalloc(paramsize * sizeof(char));
        stat |= clGetDeviceInfo(device, infotypes[i], paramsize, buf, NULL);
        if (stat != CL_SUCCESS)
            return -1;
        printf("%s\n", (char *)buf);
        free(buf);
    }
    return 0;
}

/* Returns a 2D array with the first one filled with CPUS,
 * while the second one with GPUs
 */
cl_device_id **get_devices(cl_platform_id platform)
{
    cl_device_id **device_types;
    cl_int stat;
    cl_uint numcpu, numgpu;
    cl_uint i;
    device_types = xmalloc(sizeof(cl_device_id *) * 2);

    stat = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 0, NULL, &numcpu);
    if (stat != CL_SUCCESS || numcpu == 0)
    /* No CPUs discovered or errors happened */
    cpufail:
        device_types[0] = NULL;
    else
    {
        /* CPU discovered */
        printf("[INFO]: Got %d CPU(s)\n", numcpu);
        device_types[0] = xmalloc((numcpu + 1) * sizeof(cl_device_id));
        stat = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, numcpu,
                              device_types[0], NULL);
        if (stat != CL_SUCCESS)
        {
            fprintf(stderr, "Get CPU devices failed\n");
            free(device_types[0]);
            goto cpufail;
        }
        for (i = 0; i < numcpu; ++i)
        {
            printf("[INFO]: CPU %d's info:\n", i + 1);
            print_device_info(device_types[0][i]);
        }
        device_types[0][numcpu] = NULL;
    }

    stat = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numgpu);
    if (stat != CL_SUCCESS || numgpu == 0)
    /* No GPUs discovered or errors happened */
    gpufail:
        device_types[1] = NULL;
    else
    {
        printf("[INFO]: Got %d GPU(s)\n", numgpu);
        device_types[1] = xmalloc((numgpu + 1) * sizeof(cl_device_id));
        stat = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numgpu,
                              device_types[1], NULL);
        if (stat != CL_SUCCESS)
        {
            fprintf(stderr, "Get GPU devices failed\n");
            free(device_types[1]);
            goto gpufail;
        }
        for (i = 0; i < numgpu; ++i)
        {
            printf("[INFO]: GPU %d's info:\n", i + 1);
            print_device_info(device_types[1][i]);
        }
        device_types[1][numgpu] = NULL;
    }
    return device_types;
}

void free_devices(cl_device_id **devices)
{
    free(devices[1]);
    free(devices[0]);
    free(devices);
}

int test_devices(void)
{
    size_t count;
    cl_platform_id *platforms = get_platforms();
    for (count = 0; platforms[count]; ++count)
    {
        cl_device_id **devices = get_devices(platforms[count]);
        free_devices(devices);
    }
    free(platforms);
    return 0;
}
