#include "boot/multiboot2.h"
#include "core/log.h"
#include "lib/include/libc.h"
/* Small standalone sort used instead of libc qsort (freestanding kernel) */
#include <string.h>

static inline uint32_t read_u32(const void *p) {
    uint32_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

static inline uint64_t read_u64(const void *p) {
    uint64_t v;
    memcpy(&v, p, sizeof(v));
    return v;
}

/* Minimal Multiboot2 tag structures (only fields we need). These mirror
 * the Multiboot2 spec layout for tags (type, size) followed by data.
 */
struct mb_tag {
    uint32_t type;
    uint32_t size;
};

struct mb_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    /* followed by entries: base_addr (u64), length (u64), type (u32), reserved (u32) */
};

struct mb_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    /* followed by string */
};

struct mb_tag_elf { /* section headers */
    uint32_t type;
    uint32_t size;
    uint32_t num;
    uint32_t entsize;
    uint32_t shndx;
    /* followed by ELF section headers */
};

struct mb_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t  bpp;
    uint8_t  type_fb;
    uint16_t reserved;
};

#define MB_TAG_TYPE_END 0
#define MB_TAG_TYPE_MMAP 6
#define MB_TAG_TYPE_MODULE 3
#define MB_TAG_TYPE_ELF_SECTIONS 9
#define MB_TAG_TYPE_FRAMEBUFFER 8

/* small dynamic region arrays (stack-local fixed buffers) */
#define MAX_RAW_REGIONS 32

typedef struct { uint64_t start, end; uint32_t type; } region_t;

static int region_cmp(const void *a, const void *b) {
    const region_t *ra = a, *rb = b;
    if (ra->start < rb->start) return -1;
    if (ra->start > rb->start) return 1;
    return 0;
}

static void region_sort(region_t *arr, size_t n) {
    /* insertion sort - fine for small MAX_RAW_REGIONS */
    for (size_t i = 1; i < n; i++) {
        region_t key = arr[i];
        size_t j = i;
        while (j > 0 && region_cmp(&key, &arr[j - 1]) < 0) {
            arr[j] = arr[j - 1];
            j--;
        }
        arr[j] = key;
    }
}

/* add region to list with merging for same-type adjacent regions */
static void add_region(region_t *list, size_t *count, uint64_t s, uint64_t e, uint32_t t) {
    if (s >= e) return;
    if (*count >= MAX_RAW_REGIONS) return;
    list[*count].start = s;
    list[*count].end = e;
    list[*count].type = t;
    (*count)++;
}

size_t parse_multiboot2(void *mbi, phys_mem_region_t *out, size_t max_out) {
    if (!mbi || !out) return 0;

    uint8_t *ptr = (uint8_t*)mbi;
    uint32_t total_size = *(uint32_t*)ptr; // first dword is total size in multiboot2
    LOG_DEBUG("parse_multiboot2: mbi=%p total_size=%u", mbi, (unsigned)total_size);
    if (total_size == 0) {
        LOG_DEBUG("parse_multiboot2: total_size==0, not a multiboot2 tag list");
        return 0;
    }

    region_t raw[MAX_RAW_REGIONS];
    size_t raw_count = 0;

    uint8_t *tagp = ptr + 8; /* tags start after total_size and reserved */
    uint8_t *endp = ptr + total_size;

    while (tagp < endp) {
        struct mb_tag *t = (struct mb_tag*)tagp;
        uint32_t tag_type = read_u32(&t->type);
        uint32_t tag_size = read_u32(&t->size);
        LOG_DEBUG("parse_multiboot2: tag at %p type=%u size=%u", tagp, (unsigned)tag_type, (unsigned)tag_size);
        if (tag_type == MB_TAG_TYPE_END) break;
        switch (tag_type) {
            case MB_TAG_TYPE_MMAP: {
                /* read mmap header fields safely */
                uint32_t entry_size = read_u32(tagp + offsetof(struct mb_tag_mmap, entry_size));
                uint32_t entry_version = read_u32(tagp + offsetof(struct mb_tag_mmap, entry_version));
                LOG_DEBUG("parse_multiboot2: mmap tag entry_size=%u entry_version=%u", (unsigned)entry_size, (unsigned)entry_version);
                uint8_t *ent = tagp + sizeof(struct mb_tag_mmap);
                uint8_t *end_ent = tagp + tag_size;
                while ((size_t)(end_ent - ent) >= 24) {
                    uint64_t base = read_u64(ent);
                    uint64_t len = read_u64(ent + 8);
                    uint32_t type = read_u32(ent + 16);
                    LOG_DEBUG("parse_multiboot2: mmap entry base=0x%llx len=0x%llx type=%u", (unsigned long long)base, (unsigned long long)len, (unsigned)type);
                    add_region(raw, &raw_count, base, base + len, type);
                    ent += entry_size ? entry_size : 24;
                }
            } break;
            case MB_TAG_TYPE_MODULE: {
                struct mb_tag_module *m = (struct mb_tag_module*)t;
                add_region(raw, &raw_count, (uint64_t)m->mod_start, (uint64_t)m->mod_end, 99 /* MODULE */);
            } break;
            case MB_TAG_TYPE_ELF_SECTIONS: {
                struct mb_tag_elf *e = (struct mb_tag_elf*)t;
                /* naive: treat entire tag payload as kernel reserved area */
                /* In practice parse section headers; for now reserve tag area */
                (void)e;
            } break;
            case MB_TAG_TYPE_FRAMEBUFFER: {
                struct mb_tag_framebuffer *f = (struct mb_tag_framebuffer*)t;
                add_region(raw, &raw_count, f->addr, f->addr + (uint64_t)f->pitch * f->height, 98 /* FRAMEBUFFER */);
            } break;
            default:
                break;
        }
        uint32_t sz = t->size;
        /* tags are 8-byte aligned */
        uint32_t adv = (sz + 7) & ~7;
        tagp += adv;
    }

    /* If we have no raw mmap entries, try Multiboot1-style legacy fields */
    if (raw_count == 0) {
        // try to read legacy mem_lower/mem_upper at offsets used by kmain
        uint32_t flags = *(uint32_t*)ptr; // reuse first field
        if (flags & 0x1) {
            uint32_t mem_lower = *(uint32_t*)(ptr + 4);
            uint32_t mem_upper = *(uint32_t*)(ptr + 8);
            if (mem_upper > 0) {
                add_region(raw, &raw_count, 0x100000ULL, 0x100000ULL + (uint64_t)mem_upper * 1024ULL, 1);
            }
        }
    }

    if (raw_count == 0) {
        LOG_DEBUG("parse_multiboot2: raw_count==0 after parsing tags");
        return 0;
    }

    /* canonicalize: sort by start */
    region_sort(raw, raw_count);

    /* merge adjacent/overlapping regions of same type conservatively */
    region_t merged[MAX_RAW_REGIONS];
    size_t mcount = 0;
    for (size_t i = 0; i < raw_count; i++) {
        region_t cur = raw[i];
        if (mcount == 0) { merged[mcount++] = cur; continue; }
        region_t *last = &merged[mcount - 1];
        if (cur.start <= last->end) {
            if (cur.type == last->type) {
                if (cur.end > last->end) last->end = cur.end;
            } else {
                if (cur.end > last->end) last->end = cur.end; // conservative coalescing
            }
        } else {
            merged[mcount++] = cur;
        }
    }

    /* Now produce usable regions by subtracting reserved types: we will treat
     * type==1 as usable, 3 as ACPI_RECLAIM, 4 as ACPI_NVS, 5 as BADRAM. Other
     * special types (MODULE=99, FRAMEBUFFER=98) are reserved.
     */
    size_t out_count = 0;
    /* Clamp usable regions to avoid low (<1MB) reserved areas and produce
     * usable ranges suitable for the PMM. MIN_MEMORY_START mirrors kernel's
     * expectation that the PMM manages memory from 1MB upwards. */
    const uint64_t MIN_MEMORY_START_LOCAL = 0x100000ULL;
    for (size_t i = 0; i < mcount; i++) {
        region_t *r = &merged[i];
        if (r->type == 1) {
            uint64_t s = r->start;
            uint64_t e = r->end;
            if (e <= MIN_MEMORY_START_LOCAL) continue; /* entirely below 1MB */
            if (s < MIN_MEMORY_START_LOCAL) s = MIN_MEMORY_START_LOCAL;
            if (s >= e) continue;
            if (out_count < max_out) {
                out[out_count].addr = s;
                out[out_count].len = e - s;
                out[out_count].type = 1;
                out_count++;
            }
        }
    }

    LOG_DEBUG("parse_multiboot2: produced %u usable regions", (unsigned)out_count);
    return out_count;
}
