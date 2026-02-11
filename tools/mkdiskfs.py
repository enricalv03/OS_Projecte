#!/usr/bin/env python3
"""
mkdiskfs.py -- Build a SimpleFS filesystem image and write it to the OS disk image.

Usage:
    python3 tools/mkdiskfs.py build/disk.img file1.txt file2.txt ...

On-disk layout (starting at sector 200):
    Sector 200:   SuperBlock { magic=0x53465321, version=1, num_files }
    Sector 201+:  FileEntry[N] { name[28], start_sector, size, type, reserved[8] }
    Data sectors: file contents, each file starts on a sector boundary.
"""

import struct
import sys
import os

SECTOR_SIZE = 512
DISKFS_START = 200
MAGIC = 0x53465321
VERSION = 1

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <disk.img> [file1 file2 ...]")
        sys.exit(1)

    disk_img = sys.argv[1]
    files = sys.argv[2:]

    if not os.path.exists(disk_img):
        print(f"Error: {disk_img} not found")
        sys.exit(1)

    if len(files) == 0:
        print("No files to add, skipping diskfs creation.")
        return

    # Read file contents
    file_data = []
    for f in files:
        if not os.path.exists(f):
            print(f"Warning: {f} not found, skipping")
            continue
        with open(f, 'rb') as fh:
            data = fh.read()
        name = os.path.basename(f)
        if len(name) > 27:
            name = name[:27]
        file_data.append((name, data))

    if len(file_data) == 0:
        print("No valid files found.")
        return

    num_files = len(file_data)

    # Calculate layout
    # SuperBlock: 1 sector (sector 200)
    # File entries: ceil(num_files * 48 / 512) sectors
    entry_size = 48
    entries_per_sector = SECTOR_SIZE // entry_size  # 10
    entry_sectors = (num_files + entries_per_sector - 1) // entries_per_sector

    # Data starts after superblock + entry sectors
    data_start_sector = DISKFS_START + 1 + entry_sectors

    # Build file entries and assign sectors
    entries = []
    current_data_sector = data_start_sector
    for name, data in file_data:
        size = len(data)
        sectors_needed = (size + SECTOR_SIZE - 1) // SECTOR_SIZE
        if sectors_needed == 0:
            sectors_needed = 1  # at least 1 sector even for empty files
        entries.append({
            'name': name,
            'start_sector': current_data_sector,
            'size': size,
            'type': 1,  # file
        })
        current_data_sector += sectors_needed

    # Build superblock (512 bytes)
    sb = struct.pack('<III', MAGIC, VERSION, num_files)
    sb += b'\x00' * (SECTOR_SIZE - len(sb))

    # Build file entry table
    entry_buf = bytearray()
    for e in entries:
        name_bytes = e['name'].encode('ascii')[:27]
        name_padded = name_bytes + b'\x00' * (28 - len(name_bytes))
        entry_buf += struct.pack('<28sIII8s',
                                 name_padded,
                                 e['start_sector'],
                                 e['size'],
                                 e['type'],
                                 b'\x00' * 8)
    # Pad to sector boundary
    while len(entry_buf) % SECTOR_SIZE != 0:
        entry_buf += b'\x00'

    # Write to disk image
    with open(disk_img, 'r+b') as img:
        # Write superblock
        img.seek(DISKFS_START * SECTOR_SIZE)
        img.write(sb)

        # Write entry table
        img.seek((DISKFS_START + 1) * SECTOR_SIZE)
        img.write(bytes(entry_buf))

        # Write file data
        for i, (name, data) in enumerate(file_data):
            sector = entries[i]['start_sector']
            img.seek(sector * SECTOR_SIZE)
            # Pad data to sector boundary
            padded = data + b'\x00' * (SECTOR_SIZE - (len(data) % SECTOR_SIZE))
            if len(data) % SECTOR_SIZE == 0 and len(data) > 0:
                padded = data
            img.write(padded)

    print(f"SimpleFS written to {disk_img}:")
    print(f"  SuperBlock at sector {DISKFS_START}")
    print(f"  {num_files} file(s):")
    for e in entries:
        print(f"    {e['name']:28s} sector={e['start_sector']} size={e['size']}")

if __name__ == '__main__':
    main()
