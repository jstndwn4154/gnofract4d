#ifndef FRACT_STDLIB_H_
#define FRACT_STDLIB_H_

#ifdef _WINDOWS
	#ifdef STDLIB_EXPS
		// If we're building the fract4d_stdlib module, make these functions exports.
		#define STDLIB_EAPI __declspec(dllexport)
	#else
		// Otherwise it's a no-op.
		#define STDLIB_EAPI
	#endif
#else
	#ifdef STDLIB_EXPS
		#define STDLIB_EAPI
	#else
		#define STDLIB_EAPI
	#endif
#endif

#ifdef __cplusplus
#define STDLIB_API extern "C" STDLIB_EAPI
#else
#define STDLIB_API STDLIB_EAPI
#endif

STDLIB_API void fract_rand(double *re, double *im);

typedef struct s_arena *arena_t;
STDLIB_API arena_t arena_create(int page_size, int max_pages);

STDLIB_API void arena_clear(arena_t arena);
STDLIB_API void arena_delete(arena_t arena);

STDLIB_API void *arena_alloc(arena_t arena, int element_size, int n_dimensions, int *n_elements);

STDLIB_API void array_get_int(void *allocation, int n_dimensions, int *indexes, int *pRetVal, int *pInBounds);

STDLIB_API void array_get_double(void *allocation, int n_dimensions, int *indexes, double *pRetVal, int *pInBounds);

STDLIB_API int array_set_int(void *allocation, int n_dimensions, int *indexes, int val);

STDLIB_API int array_set_double(void *allocation, int n_dimensions, int *indexes, double val);

STDLIB_API void *alloc_array1D(arena_t arena, int element_size, int size);
STDLIB_API void *alloc_array2D(arena_t arena, int element_size, int xsize, int ysize);
STDLIB_API void *alloc_array3D(arena_t arena, int element_size, int xsize, int ysize, int zsize);
STDLIB_API void *alloc_array4D(arena_t arena, int element_size, int xsize, int ysize, int zsize, int wsize);

STDLIB_API int read_int_array_1D(void *array, int x);
STDLIB_API int write_int_array_1D(void *array, int x, int val);

STDLIB_API int read_int_array_2D(void *array, int x, int y);
STDLIB_API int write_int_array_2D(void *array, int x, int y, int val);

STDLIB_API double read_float_array_1D(void *array, int x);
STDLIB_API int write_float_array_1D(void *array, int x, double val);

STDLIB_API double read_float_array_2D(void *array, int x, int y);
STDLIB_API int write_float_array_2D(void *array, int x, int y, double val);

#endif
