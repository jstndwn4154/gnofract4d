// The following is to make the setup system happy under Windows:
#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif

extern "C" PyMODINIT_FUNC initfract4d_stdlib()
{
	// Dummy routine that Python can call to get this library, but which doesn't do anything.
}

#pragma comment(linker, "/EXPORT:_fract_rand")
#pragma comment(linker, "/EXPORT:_arena_create")
#pragma comment(linker, "/EXPORT:_arena_clear")
#pragma comment(linker, "/EXPORT:_arena_delete")
#pragma comment(linker, "/EXPORT:_arena_alloc")
#pragma comment(linker, "/EXPORT:_array_get_int")
#pragma comment(linker, "/EXPORT:_array_get_double")
#pragma comment(linker, "/EXPORT:_array_set_int")
#pragma comment(linker, "/EXPORT:_array_set_double")
#pragma comment(linker, "/EXPORT:_alloc_array1D")
#pragma comment(linker, "/EXPORT:_alloc_array2D")
#pragma comment(linker, "/EXPORT:_alloc_array3D")
#pragma comment(linker, "/EXPORT:_alloc_array4D")
#pragma comment(linker, "/EXPORT:_read_int_array_1D")
#pragma comment(linker, "/EXPORT:_write_int_array_1D")
#pragma comment(linker, "/EXPORT:_read_int_array_2D")
#pragma comment(linker, "/EXPORT:_write_int_array_2D")
#pragma comment(linker, "/EXPORT:_read_float_array_1D")
#pragma comment(linker, "/EXPORT:_write_float_array_1D")
#pragma comment(linker, "/EXPORT:_read_float_array_2D")
#pragma comment(linker, "/EXPORT:_write_float_array_2D")
