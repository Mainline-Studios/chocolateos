#pragma once

#include <choc/types.h>
#include <limine.h>

void pmm_init(struct limine_memmap_response *map, uint64_t hhdm);
uint64_t pmm_usable_bytes(void);
uint64_t pmm_total_reported_bytes(void);
void pmm_dump(void);
uint64_t hhdm_offset(void);
void *pmm_to_virt(uint64_t phys);
uint64_t pmm_to_phys(const void *virt);
void *pmm_alloc(size_t bytes, size_t align);
