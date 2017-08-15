#ifndef _BCACHE_SUPER_IO_H
#define _BCACHE_SUPER_IO_H

#include "extents.h"
#include "eytzinger.h"
#include "super_types.h"
#include "super.h"

#include <asm/byteorder.h>

struct bch_sb_field *bch2_sb_field_get(struct bch_sb *, enum bch_sb_field_type);
struct bch_sb_field *bch2_sb_field_resize(struct bcache_superblock *,
					 enum bch_sb_field_type, unsigned);
struct bch_sb_field *bch2_fs_sb_field_resize(struct bch_fs *,
					 enum bch_sb_field_type, unsigned);

#define field_to_type(_f, _name)					\
	container_of_or_null(_f, struct bch_sb_field_##_name, field)

#define BCH_SB_FIELD_TYPE(_name)					\
static inline struct bch_sb_field_##_name *				\
bch2_sb_get_##_name(struct bch_sb *sb)					\
{									\
	return field_to_type(bch2_sb_field_get(sb,			\
				BCH_SB_FIELD_##_name), _name);		\
}									\
									\
static inline struct bch_sb_field_##_name *				\
bch2_sb_resize_##_name(struct bcache_superblock *sb, unsigned u64s)	\
{									\
	return field_to_type(bch2_sb_field_resize(sb,			\
				BCH_SB_FIELD_##_name, u64s), _name);	\
}									\
									\
static inline struct bch_sb_field_##_name *				\
bch2_fs_sb_resize_##_name(struct bch_fs *c, unsigned u64s)		\
{									\
	return field_to_type(bch2_fs_sb_field_resize(c,			\
				BCH_SB_FIELD_##_name, u64s), _name);	\
}

BCH_SB_FIELD_TYPE(journal);
BCH_SB_FIELD_TYPE(members);
BCH_SB_FIELD_TYPE(crypt);
BCH_SB_FIELD_TYPE(replicas);

static inline bool bch2_dev_exists(struct bch_sb *sb,
				   struct bch_sb_field_members *mi,
				   unsigned dev)
{
	return dev < sb->nr_devices &&
		!bch2_is_zero(mi->members[dev].uuid.b, sizeof(uuid_le));
}

static inline bool bch2_sb_test_feature(struct bch_sb *sb,
					enum bch_sb_features f)
{
	unsigned w = f / 64;
	unsigned b = f % 64;

	return le64_to_cpu(sb->features[w]) & (1ULL << b);
}

static inline void bch2_sb_set_feature(struct bch_sb *sb,
				       enum bch_sb_features f)
{
	if (!bch2_sb_test_feature(sb, f)) {
		unsigned w = f / 64;
		unsigned b = f % 64;

		le64_add_cpu(&sb->features[w], 1ULL << b);
	}
}

static inline __le64 bch2_sb_magic(struct bch_fs *c)
{
	__le64 ret;
	memcpy(&ret, &c->sb.uuid, sizeof(ret));
	return ret;
}

static inline __u64 jset_magic(struct bch_fs *c)
{
	return __le64_to_cpu(bch2_sb_magic(c) ^ JSET_MAGIC);
}

static inline __u64 pset_magic(struct bch_fs *c)
{
	return __le64_to_cpu(bch2_sb_magic(c) ^ PSET_MAGIC);
}

static inline __u64 bset_magic(struct bch_fs *c)
{
	return __le64_to_cpu(bch2_sb_magic(c) ^ BSET_MAGIC);
}

static inline struct bch_member_cpu bch2_mi_to_cpu(struct bch_member *mi)
{
	return (struct bch_member_cpu) {
		.nbuckets	= le64_to_cpu(mi->nbuckets),
		.first_bucket	= le16_to_cpu(mi->first_bucket),
		.bucket_size	= le16_to_cpu(mi->bucket_size),
		.state		= BCH_MEMBER_STATE(mi),
		.tier		= BCH_MEMBER_TIER(mi),
		.replacement	= BCH_MEMBER_REPLACEMENT(mi),
		.discard	= BCH_MEMBER_DISCARD(mi),
		.valid		= !bch2_is_zero(mi->uuid.b, sizeof(uuid_le)),
	};
}

int bch2_sb_to_fs(struct bch_fs *, struct bch_sb *);
int bch2_sb_from_fs(struct bch_fs *, struct bch_dev *);

void bch2_free_super(struct bcache_superblock *);
int bch2_super_realloc(struct bcache_superblock *, unsigned);

const char *bch2_sb_validate_journal(struct bch_sb *,
					 struct bch_member_cpu);
const char *bch2_sb_validate(struct bcache_superblock *);

const char *bch2_read_super(struct bcache_superblock *,
			   struct bch_opts, const char *);
void bch2_write_super(struct bch_fs *);

bool bch2_sb_has_replicas(struct bch_fs *, struct bkey_s_c_extent,
			  enum bch_data_types);
int bch2_check_mark_super(struct bch_fs *, struct bkey_s_c_extent,
			  enum bch_data_types);

struct replicas_status {
	struct {
		unsigned	nr_online;
		unsigned	nr_offline;
	}			replicas[BCH_DATA_NR];
};

struct replicas_status __bch2_replicas_status(struct bch_fs *,
					      struct bch_devs_mask);
struct replicas_status bch2_replicas_status(struct bch_fs *);
bool bch2_have_enough_devs(struct bch_fs *, struct replicas_status, unsigned);

unsigned bch2_replicas_online(struct bch_fs *, bool);
unsigned bch2_dev_has_data(struct bch_fs *, struct bch_dev *);

int bch2_replicas_gc_end(struct bch_fs *, int);
int bch2_replicas_gc_start(struct bch_fs *, unsigned);

#endif /* _BCACHE_SUPER_IO_H */