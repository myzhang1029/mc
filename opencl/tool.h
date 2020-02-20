char *file2str(const char *restrict fn);
cl_platform_id *get_platforms(void);
cl_device_id **get_devices(cl_platform_id platform);
void free_devices(cl_device_id **devices);
