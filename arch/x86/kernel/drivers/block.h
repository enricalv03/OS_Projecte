#ifndef BLOCK_H
#define BLOCK_H

/* Block Device Interface - Abstraction layer for disk access */

/* Standard block size (512 bytes) */
#define BLOCK_SIZE 512

/* Block device operations structure */
typedef struct block_device {
    /* Device name */
    const char* name;
    
    /* Block size in bytes (usually 512) */
    unsigned int block_size;
    
    /* Total number of blocks */
    unsigned int total_blocks;
    
    /* Read blocks from device */
    int (*read)(struct block_device* dev, unsigned int block, unsigned int count, void* buffer);
    
    /* Write blocks to device */
    int (*write)(struct block_device* dev, unsigned int block, unsigned int count, const void* buffer);
    
    /* Write blocks to device (synchronous, ensures data is written) */
    int (*write_sync)(struct block_device* dev, unsigned int block, unsigned int count, const void* buffer);
    
    /* Private data for the device driver */
    void* private_data;
} block_device_t;

/* Register a block device */
int block_device_register(block_device_t* device);

/* Get block device by name */
block_device_t* block_device_get(const char* name);

/* Read blocks from a registered device */
int block_read(const char* device_name, unsigned int block, unsigned int count, void* buffer);

/* Write blocks to a registered device */
int block_write(const char* device_name, unsigned int block, unsigned int count, const void* buffer);

/* Initialize block device subsystem */
void block_init(void);

#endif

