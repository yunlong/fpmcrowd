#ifndef FPM_SHM_H
#define FPM_SHM_H 1

struct fpm_shm_s;

struct fpm_shm_s {
	struct fpm_shm_s *prev, *next;
	void *mem;
	size_t sz;
	size_t used;
};

struct fpm_shm_s *fpm_shm_alloc(size_t sz);
void fpm_shm_free_list(struct fpm_shm_s *, void *);
void *fpm_shm_alloc_chunk(struct fpm_shm_s **head, size_t sz, void **mem);

#endif

