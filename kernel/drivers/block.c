#include "block.h"
#include "ata.h"

/* Maximum number of registered block devices */
#define MAX_BLOCK_DEVICES 8

static block_device_t* block_devices[MAX_BLOCK_DEVICES];
static unsigned int num_block_devices = 0;

/* ATA block device structure */
static block_device_t ata_block_device;

/* ATA read function wrapper */
static int ata_block_read(block_device_t* dev, unsigned int block, unsigned int count, void* buffer) {
    (void)dev; /* Unused */
    
    /* Convert block number to LBA (they're the same for ATA) */
    unsigned int lba = block;
    
    /* Limit count to 255 sectors (ATA limitation) */
    if (count > 255) {
        count = 255;
    }
    
    return ata_read_sectors(lba, (unsigned char)count, buffer);
}

/* ATA write function wrapper */
static int ata_block_write(block_device_t* dev, unsigned int block, unsigned int count, const void* buffer) {
    (void)dev; /* Unused */
    
    unsigned int lba = block;
    
    if (count > 255) {
        count = 255;
    }
    
    return ata_write_sectors(lba, (unsigned char)count, buffer);
}

/* ATA write sync (same as write for now, but ensures data is flushed) */
static int ata_block_write_sync(block_device_t* dev, unsigned int block, unsigned int count, const void* buffer) {
    int result = ata_block_write(dev, block, count, buffer);
    if (result == 0) {
        /* Flush cache to ensure data is written */
        /* This is already done in ata_write_sectors, but we keep it explicit */
        return 0;
    }
    return result;
}

/* Initialize block device subsystem */
void block_init(void) {
    /* Clear device list */
    for (unsigned int i = 0; i < MAX_BLOCK_DEVICES; i++) {
        block_devices[i] = 0;
    }
    num_block_devices = 0;
    
    /* Initialize ATA if available */
    if (ata_init() == 0) {
        /* Set up ATA block device */
        ata_block_device.name = "ata0";
        ata_block_device.block_size = BLOCK_SIZE;
        ata_block_device.total_blocks = 0; /* Will be set after identify */
        ata_block_device.read = ata_block_read;
        ata_block_device.write = ata_block_write;
        ata_block_device.write_sync = ata_block_write_sync;
        ata_block_device.private_data = 0;
        
        /* Try to identify device to get size */
        unsigned short identify_buffer[256];
        if (ata_identify(identify_buffer) == 0) {
            /* Extract LBA28 capacity from identify data */
            /* Word 60-61 contain LBA28 capacity (little endian) */
            unsigned int lba_low = identify_buffer[60];
            unsigned int lba_high = identify_buffer[61];
            unsigned int lba28_capacity = lba_low | (lba_high << 16);
            
            /* For now, use a default if identify fails or returns 0 */
            if (lba28_capacity == 0) {
                lba28_capacity = 2880; /* Default: 1.44 MB floppy size */
            }
            
            ata_block_device.total_blocks = lba28_capacity;
        } else {
            /* If identify fails, use default size */
            ata_block_device.total_blocks = 2880;
        }
        
        /* Register ATA device */
        block_device_register(&ata_block_device);
    }
}

/* Register a block device */
int block_device_register(block_device_t* device) {
    if (!device || num_block_devices >= MAX_BLOCK_DEVICES) {
        return -1;
    }
    
    block_devices[num_block_devices++] = device;
    return 0;
}

/* Get block device by name */
block_device_t* block_device_get(const char* name) {
    if (!name) {
        return 0;
    }
    
    for (unsigned int i = 0; i < num_block_devices; i++) {
        if (block_devices[i] && block_devices[i]->name) {
            /* Simple string comparison */
            const char* dev_name = block_devices[i]->name;
            unsigned int j = 0;
            while (name[j] != 0 && dev_name[j] != 0 && name[j] == dev_name[j]) {
                j++;
            }
            if (name[j] == 0 && dev_name[j] == 0) {
                return block_devices[i];
            }
        }
    }
    
    return 0;
}

/* Read blocks from a registered device */
int block_read(const char* device_name, unsigned int block, unsigned int count, void* buffer) {
    block_device_t* dev = block_device_get(device_name);
    if (!dev || !dev->read) {
        return -1;
    }
    
    return dev->read(dev, block, count, buffer);
}

/* Write blocks to a registered device */
int block_write(const char* device_name, unsigned int block, unsigned int count, const void* buffer) {
    block_device_t* dev = block_device_get(device_name);
    if (!dev || !dev->write) {
        return -1;
    }
    
    return dev->write(dev, block, count, buffer);
}

