#include "qemu/osdep.h"
#include "block/block_int.h"
#include "block/qcow2.h"
#include "block/pierre.h"

/* we sure of this? */
#define SECTOR_SIZE 512

#define DEFAULT_L2_CACHE_ENTRY_SIZE 65536

/* Copied from qcow2-cache.c */
typedef struct Qcow2CachedTable {
int64_t  offset;
uint64_t lru_counter;
int      ref;
bool     dirty;
} Qcow2CachedTable;

struct Qcow2Cache {
Qcow2CachedTable       *entries;
struct Qcow2Cache      *depends;
int                     size;
int                     table_size;
bool                    depends_on_flush;
void                   *table_array;
uint64_t                lru_counter;
uint64_t                cache_clean_lru_counter;
};

int resize_cache(BlockDriverState *bs, int size);

int pierre_map(BlockDriverState *bs) {
    int64_t total_size = bs->total_sectors * SECTOR_SIZE;
    BDRVQcow2State *s = bs->opaque;
    int cluster_size = s->cluster_size;
    int64_t total_clusters = total_size/cluster_size;

    char *secmap = malloc(total_clusters);
    if(!secmap) {
        fprintf(stderr, "ERROR: cannot allocate memory\n");
        exit(-1);
    }
    bzero(secmap, total_clusters);

    BlockDriverState *curbs = bs;
    while(1) {
        s = curbs->opaque;

        uint64_t clusters_normal = 0;
        uint64_t clusters_unallocated = 0;
        uint64_t clusters_zero_plain = 0;
        uint64_t clusters_zero_alloc = 0;
        uint64_t clusters_compressed = 0;

        printf("disk %s\n", curbs->exact_filename);
        struct Qcow2Cache *c = s->l2_table_cache;
        printf("Cache size: %d, table size: %d\n", c->size, c->table_size);

        /* Check that the size is consistent with the base disc */
        if(curbs->total_sectors*SECTOR_SIZE != total_size) {
            fprintf(stderr, "ERROR: layer %s has different size (%ld) from "
                    "base disc %s (size %ld)\n", curbs->exact_filename,
                    curbs->total_sectors*SECTOR_SIZE, bs->exact_filename,
                    total_size);
            exit(-1);
        }

        /* that the cluster size is the same */
        if(s->cluster_size != cluster_size) {
            fprintf(stderr, "ERROR: layer %s has different cluster size (%d) "
                    "from base disc %s (cluster size %d)\n",
                    curbs->exact_filename, s->cluster_size, bs->exact_filename,
                    cluster_size);
            exit(-1);
        }

        for(int64_t cluster = 0; cluster < total_clusters; cluster++) {
            unsigned int cur_bytes = s->cluster_size;
            uint64_t cluster_offset;

            if(secmap[cluster])
                continue;

            qemu_co_mutex_lock(&s->lock);
            int ret = qcow2_get_cluster_offset(curbs, cluster*s->cluster_size,
                    &cur_bytes, &cluster_offset);
            qemu_co_mutex_unlock(&s->lock);

            switch(ret) {
                case QCOW2_CLUSTER_UNALLOCATED:
                    clusters_unallocated++;
                    break;
                case QCOW2_CLUSTER_ZERO_PLAIN:
                    clusters_zero_plain++;
                    break;
                case QCOW2_CLUSTER_ZERO_ALLOC:
                    clusters_zero_alloc++;
                    break;
                case QCOW2_CLUSTER_NORMAL:
                    clusters_normal++;
                    assert(!secmap[cluster]);
                    secmap[cluster] = (char)1;
                    break;
                case QCOW2_CLUSTER_COMPRESSED:
                    clusters_compressed++;
                    break;
            }
        }

        printf("- unallocated: %lu\n", clusters_unallocated);
        printf("- normal: %lu\n", clusters_normal);
        printf("- zero_plain: %lu\n", clusters_zero_plain);
        printf("- zero_alloc: %lu\n", clusters_zero_alloc);
        printf("- compressed: %lu\n", clusters_compressed);

        int new_cache_size = MAX(1, (clusters_normal * 40) / total_clusters);
        printf("- NEW CACHE SIZE: %d\n", new_cache_size);
        resize_cache(curbs, new_cache_size);

        /* Next layer */
        if(curbs->backing)
            curbs = curbs->backing->bs;
        else
            break;
    }

    free(secmap);
    sleep(5);
    return 0;
}

int resize_cache(BlockDriverState *bs, int size) {
    BDRVQcow2State *s = bs->opaque;
    struct Qcow2Cache *c = s->l2_table_cache;

    printf("Cache size: %d, table size: %d\n", c->size, c->table_size);
    qcow2_cache_destroy(s->l2_table_cache);
    s-> l2_table_cache = qcow2_cache_create(bs, size,
            DEFAULT_L2_CACHE_ENTRY_SIZE);

    return 0;
}
