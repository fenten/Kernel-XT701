#ifndef _LINUX_SLQB_DEF_H
#define _LINUX_SLQB_DEF_H

/*
 * SLQB : A slab allocator with object queues.
 *
 * (C) 2008 Nick Piggin <npiggin@suse.de>
 */
#include <linux/types.h>
#include <linux/gfp.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/rcu_types.h>
#include <linux/mm_types.h>
#include <linux/kernel.h>

enum stat_item {
	ALLOC_LOCAL,
	ALLOC_OFFNODE,
	ALLOC_FAILED,
	ALLOC_NEWPAGE,
	ALLOC_PROCESS_RFREE,
	ALLOC_FREEPAGE,
	FREE_LOCAL,
	FREE_REMOTE,
	FREE_FLUSH_RCACHE,
	FREE_FREEPAGE,
	NR_SLQB_STAT_ITEMS
};

struct kmem_cache_list;

struct kmlist {
	unsigned long nr;
	void **head, **tail;
};

struct kmem_cache_remote_free {
	spinlock_t lock;
	struct kmlist list;
} ____cacheline_aligned;

struct kmem_cache_list {
	struct kmlist freelist;
#ifdef CONFIG_SMP
	int remote_free_check;
#endif

	unsigned long nr_partial;
	struct list_head partial;

	unsigned long nr_slabs;

	struct kmem_cache *cache;
	//struct list_head full;

#ifdef CONFIG_SMP
	struct kmem_cache_remote_free remote_free;
#endif
} ____cacheline_aligned;

struct kmem_cache_cpu {
	struct kmem_cache_list list;

#ifdef CONFIG_SMP
	struct kmlist rlist;
	struct kmem_cache_list *remote_cache_list;
#endif

#ifdef CONFIG_SLQB_STATS
	unsigned stat[NR_SLQB_STAT_ITEMS];
#endif
} ____cacheline_aligned;

struct kmem_cache_node {
	struct kmem_cache_list list;
	spinlock_t list_lock;	/* Protect partial list and nr_partial */
} ____cacheline_aligned;

/*
 * Management object for a slab cache.
 */
struct kmem_cache {
	/* Used for retriving partial slabs etc */
	unsigned long flags;
	int batch;		/* Freeing batch size */
	int size;		/* The size of an object including meta data */
	int objsize;		/* The size of an object without meta data */
	int offset;		/* Free pointer offset. */
	int order;

	/* Allocation and freeing of slabs */
	int objects;		/* Number of objects in slab */
	gfp_t allocflags;	/* gfp flags to use on each alloc */
	void (*ctor)(void *);
	int inuse;		/* Offset to metadata */
	int align;		/* Alignment */
	const char *name;	/* Name (only for display!) */
	struct list_head list;	/* List of slab caches */

#ifdef CONFIG_NUMA
	/*
	 * Defragmentation by allocating from a remote node.
	 */
	int remote_node_defrag_ratio;
	struct kmem_cache_node *node[MAX_NUMNODES];
#endif
#ifdef CONFIG_SMP
	struct kmem_cache_cpu *cpu_slab[NR_CPUS];
#else
	struct kmem_cache_cpu cpu_slab;
#endif
};

/*
 * Kmalloc subsystem.
 */
#if defined(ARCH_KMALLOC_MINALIGN) && ARCH_KMALLOC_MINALIGN > 8
#define KMALLOC_MIN_SIZE ARCH_KMALLOC_MINALIGN
#else
#define KMALLOC_MIN_SIZE 8
#endif

#define KMALLOC_SHIFT_LOW ilog2(KMALLOC_MIN_SIZE)
#define KMALLOC_SHIFT_SLQB_HIGH (PAGE_SHIFT + 9)

/*
 * We keep the general caches in an array of slab caches that are used for
 * 2^x bytes of allocations.
 */
extern struct kmem_cache kmalloc_caches[KMALLOC_SHIFT_SLQB_HIGH + 1];
extern struct kmem_cache kmalloc_caches_dma[KMALLOC_SHIFT_SLQB_HIGH + 1];

/*
 * Sorry that the following has to be that ugly but some versions of GCC
 * have trouble with constant propagation and loops.
 */
static __always_inline int kmalloc_index(size_t size)
{
	if (unlikely(!size))
		return 0;
	if (unlikely(size > 1UL << KMALLOC_SHIFT_SLQB_HIGH))
		return 0;

	if (unlikely(size <= KMALLOC_MIN_SIZE))
		return KMALLOC_SHIFT_LOW;

#if L1_CACHE_BYTES < 64
	if (size > 64 && size <= 96)
		return 1;
#endif
#if L1_CACHE_BYTES < 128
	if (size > 128 && size <= 192)
		return 2;
#endif
	if (size <=          8) return 3;
	if (size <=         16) return 4;
	if (size <=         32) return 5;
	if (size <=         64) return 6;
	if (size <=        128) return 7;
	if (size <=        256) return 8;
	if (size <=        512) return 9;
	if (size <=       1024) return 10;
	if (size <=   2 * 1024) return 11;
	if (size <=   4 * 1024) return 12;
	if (size <=   8 * 1024) return 13;
	if (size <=  16 * 1024) return 14;
	if (size <=  32 * 1024) return 15;
	if (size <=  64 * 1024) return 16;
	if (size <= 128 * 1024) return 17;
	if (size <= 256 * 1024) return 18;
	if (size <= 512 * 1024) return 19;
	if (size <= 1024 * 1024) return 20;
	if (size <=  2 * 1024 * 1024) return 21;
	return -1;

/*
 * What we really wanted to do and cannot do because of compiler issues is:
 *	int i;
 *	for (i = KMALLOC_SHIFT_LOW; i <= KMALLOC_SHIFT_HIGH; i++)
 *		if (size <= (1 << i))
 *			return i;
 */
}

#ifdef CONFIG_ZONE_DMA
#define SLQB_DMA __GFP_DMA
#else
/* Disable DMA functionality */
#define SLQB_DMA (__force gfp_t)0
#endif

/*
 * Find the slab cache for a given combination of allocation flags and size.
 *
 * This ought to end up with a global pointer to the right cache
 * in kmalloc_caches.
 */
static __always_inline struct kmem_cache *kmalloc_slab(size_t size, gfp_t flags)
{
	int index = kmalloc_index(size);

	if (unlikely(index == 0))
		return NULL;

	if (likely(!(flags & SLQB_DMA)))
		return &kmalloc_caches[index];
	else
		return &kmalloc_caches_dma[index];
}

void *kmem_cache_alloc(struct kmem_cache *, gfp_t);
void *__kmalloc(size_t size, gfp_t flags);

#ifndef ARCH_KMALLOC_MINALIGN
#define ARCH_KMALLOC_MINALIGN __alignof__(unsigned long long)
#endif

#ifndef ARCH_SLAB_MINALIGN
#define ARCH_SLAB_MINALIGN __alignof__(unsigned long long)
#endif

#define KMALLOC_HEADER (ARCH_KMALLOC_MINALIGN < sizeof(void *) ? sizeof(void *) : ARCH_KMALLOC_MINALIGN)

static __always_inline void *kmalloc(size_t size, gfp_t flags)
{
	if (__builtin_constant_p(size)) {
		struct kmem_cache *s;

		s = kmalloc_slab(size, flags);
		if (unlikely(ZERO_OR_NULL_PTR(s)))
			return s;

		return kmem_cache_alloc(s, flags);
	}
	return __kmalloc(size, flags);
}

#ifdef CONFIG_NUMA
void *__kmalloc_node(size_t size, gfp_t flags, int node);
void *kmem_cache_alloc_node(struct kmem_cache *, gfp_t flags, int node);

static __always_inline void *kmalloc_node(size_t size, gfp_t flags, int node)
{
	if (__builtin_constant_p(size)) {
		struct kmem_cache *s;

		s = kmalloc_slab(size, flags);
		if (unlikely(ZERO_OR_NULL_PTR(s)))
			return s;

		return kmem_cache_alloc_node(s, flags, node);
	}
	return __kmalloc_node(size, flags, node);
}
#endif

#endif /* _LINUX_SLQB_DEF_H */
