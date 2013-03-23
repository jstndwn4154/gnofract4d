// The following is to make the setup system happy under Windows:
#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif

extern "C" PyMODINIT_FUNC initfract4d_stdlib()
{
	// Dummy routine that Python can call to get this library, but which doesn't do anything.
}

extern "C"
{
	__declspec(dllexport) void fract_rand(double *re, double *im);
	__declspec(dllexport) typedef struct s_arena *arena_t;
	__declspec(dllexport) arena_t arena_create(int page_size, int max_pages);
	__declspec(dllexport) void arena_clear(arena_t arena);
	__declspec(dllexport) void arena_delete(arena_t arena);
	__declspec(dllexport) void *arena_alloc(arena_t arena, int element_size, int n_dimensions, int *n_elements);
	__declspec(dllexport) void array_get_int(void *allocation, int n_dimensions, int *indexes, int *pRetVal, int *pInBounds);
	__declspec(dllexport) void array_get_double(void *allocation, int n_dimensions, int *indexes, double *pRetVal, int *pInBounds);
	__declspec(dllexport) int array_set_int(void *allocation, int n_dimensions, int *indexes, int val);
	__declspec(dllexport) int array_set_double(void *allocation, int n_dimensions, int *indexes, double val);
	__declspec(dllexport) void *alloc_array1D(arena_t arena, int element_size, int size);
	__declspec(dllexport) void *alloc_array2D(arena_t arena, int element_size, int xsize, int ysize);
	__declspec(dllexport) void *alloc_array3D(arena_t arena, int element_size, int xsize, int ysize, int zsize);
	__declspec(dllexport) void *alloc_array4D(arena_t arena, int element_size, int xsize, int ysize, int zsize, int wsize);
	__declspec(dllexport) int read_int_array_1D(void *array, int x);
	__declspec(dllexport) int write_int_array_1D(void *array, int x, int val);
	__declspec(dllexport) int read_int_array_2D(void *array, int x, int y);
	__declspec(dllexport) int write_int_array_2D(void *array, int x, int y, int val);
	__declspec(dllexport) double read_float_array_1D(void *array, int x);
	__declspec(dllexport) int write_float_array_1D(void *array, int x, double val);
	__declspec(dllexport) double read_float_array_2D(void *array, int x, int y);
	__declspec(dllexport) int write_float_array_2D(void *array, int x, int y, double val);
}