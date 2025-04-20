#pragma once
#include <kernel/types.h>

struct blockdevice;

extern struct blockdevice virtio_bd;
void init_virtio_bd(void) ;
int32 virtio_read_blocks(struct blockdevice* bdev, void* buffer, sector_t sector, size_t count);
int32 virtio_write_blocks(struct blockdevice* bdev,const void* buffer, sector_t sector, size_t count);