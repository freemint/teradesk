#if !defined(__XATTR) && !defined(__KERNEL__) && !defined(__KERNEL_MODULE__)
#define __XATTR
typedef struct xattr
{
	unsigned short st_mode;
	long           st_ino;	/* must be 32 bits */
	unsigned short st_dev;	/* must be 16 bits */
	unsigned short st_rdev;	/* not supported by the kernel */
	unsigned short st_nlink;
	unsigned short st_uid;	/* must be 16 bits */
	unsigned short st_gid;	/* must be 16 bits */
	long           st_size;
	long           st_blksize;
	long           st_blocks;
	struct {
		union {
			unsigned long  tv_sec; /* actually time&date in DOSTIME format */
			struct {
				unsigned short time;
				unsigned short date;
			} d;
		} u;
	} st_mtim;
#define st_mtime   st_mtim.u.tv_sec
	struct {
		union {
			unsigned long  tv_sec; /* actually time&date in DOSTIME format */
			struct {
				unsigned short time;
				unsigned short date;
			} d;
		} u;
	} st_atim;
#define st_atime   st_atim.u.tv_sec
	struct {
		union {
			unsigned long  tv_sec; /* actually time&date in DOSTIME format */
			struct {
				unsigned short time;
				unsigned short date;
			} d;
		} u;
	} st_ctim;
#define st_ctime   st_ctim.u.tv_sec
	short          st_attr;
	short res1;		/* reserved for future kernel use */
	long res2[2];
} XATTR;
#define FSTAT		(('F'<< 8) | 0)
#endif

