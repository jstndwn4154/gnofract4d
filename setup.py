#!/usr/bin/env python

import shutil
from distutils.core import setup, Extension
import distutils.sysconfig
import os
import stat
import commands
import sys

gnofract4d_version = '4.0'

if float(sys.version[:3]) < 2.4:
	print "Sorry, you need Python 2.4 or higher to run Gnofract 4D."
	print "You have version %s. Please upgrade." % sys.version
	sys.exit(1)

# hack to use a different Python for building if an env var is set
# I use this to build python-2.2 and 2.4 RPMs.
build_version = os.environ.get("BUILD_PYTHON_VERSION")
build_python = os.environ.get("BUILD_PYTHON")

if build_version and build_python and sys.version[:3] != build_version:
	print sys.version[:3], build_version
	args = ["/usr/bin/python"] + sys.argv
	print "running other Python version %s with args: %s" % (build_python,args)
	os.execv(build_python, args)

from buildtools import my_bdist_rpm, my_build, my_build_ext, my_install_lib, my_install_egg_info

# Extensions need to link against appropriate libs
# We use pkg-config to find the appropriate set of includes and libs

def call_package_config(package,option,optional=False):
	'''invoke pkg-config, if it exists, to find the appropriate
	arguments for a library'''
	cmd = "pkg-config %s %s" % (package, option)
	(status,output) = commands.getstatusoutput(cmd)
	if status != 0:
		if optional:
			print >>sys.stderr, "Can't find '%s'" % package
			print >>sys.stderr, "Some functionality will be disabled"
			return []
		else:
			print >>sys.stderr, "Can't set up. Error running '%s'." % cmd
			print >>sys.stderr, output
			print >>sys.stderr, "Possibly you don't have one of these installed: '%s'." % package
			sys.exit(1)

	return output.split()

extra_macros = []

if 'win' == sys.platform[:3]:
	from win32api import GetLogicalDriveStrings

	def scan_for_file(file, BaseDir = "", joinFile = False):
		def file_scanner(out, dirpath, files):
			if file in files:
				if joinFile:
					out += [ os.path.join(dirpath, file) ]
				else:
					out += [ dirpath ]
				return out

		Out = []
		for drive in GetLogicalDriveStrings().split('\0'):
			os.path.walk(os.path.join(drive, BaseDir), file_scanner, Out)
			if Out != []:
				break
		if Out == []:
			print >>sys.stderr, "FILE NOT FOUND (%s)" % file
			return []
		else:
			return Out

	cache_file = os.path.join(os.getcwd(), 'setup.cache')
	if os.path.exists(cache_file) == False or os.path.getsize(cache_file) == 0:
		cache_file = open(cache_file, 'w')
		png_flags = scan_for_file('png.h', os.path.join('noarch','GnuWin32'))
		if png_flags != []:
			for i in xrange(0, len(png_flags)):
				png_flags[i] = "/I" + png_flags[i]
			cache_file.write(" ".join(png_flags) + '\n')
		if "64 bit" in sys.version:
			png_libs = scan_for_file('libpng16.lib', os.path.join('x86_64','GnuWin64'), True)
		else:
			png_libs = scan_for_file('libpng.lib', os.path.join('x86','GnuWin32'), True)
		if png_libs != []:
			cache_file.write("".join(png_libs) + '\n')
		jpg_flags = scan_for_file('jpeglib.h', os.path.join('x86','GnuWin32'))
		if jpg_flags != []:
			for i in xrange(0, len(jpg_flags)):
				jpg_flags[i] = "/I" + jpg_flags[i]
			cache_file.write(" ".join(jpg_flags) + '\n')
		if "64 bit" in sys.version:
			jpg_libs = scan_for_file('libjpeg.lib', os.path.join('x86_64','GnuWin64'), True)
		else:
			jpg_libs = scan_for_file('libjpeg.lib', os.path.join('x86','GnuWin32'), True)
		if jpg_libs != []:
			jpg_libs = [ 'libjpeg' ]
			cache_file.write("".join(jpg_libs))
		# cache_file.writelines(png_flags + '\n' + png_libs + '\n' + jpg_flags + '\n' + jpg_libs)
		cache_file.close()
	else:
		cache_file = open(cache_file)
		lines = cache_file.readlines()
		png_flags = [ lines[0].strip() ]
		png_libs = [ lines[1].strip() ]
		jpg_flags = [ lines[2].strip() ]
		jpg_libs = [ lines[3].strip() ]
		cache_file.close()
		del lines
#		def scan_for_libjpeg_h(jpg_libs):
#			for drive in GetLogicalDriveStrings().split('\0'):
#				for (root, dirs, files) in os.walk(drive + 'Program Files\\'):
#					if len(files) == 0:
#						continue
#					if 'jpeglib.h' in files:
#						jpg_libs += [ root + '\\jpeglib.h' ]
#						return jpg_libs
#			if jpg_libs == []:
#				print "NO JPEG HEADERS FOUND"
	if png_libs != []:
		extra_macros.append(('PNG_ENABLED', 1))
	if jpg_libs != []:
		extra_macros.append(('JPG_ENABLED', 1))

else:
	png_flags = call_package_config("libpng", "--cflags", True)
	if png_flags != []:
		extra_macros.append(('PNG_ENABLED', 1))
	else:
		print "NO PNG HEADERS FOUND"
		
	png_libs = call_package_config("libpng", "--libs", True)

	jpg_lib = "jpeg"
	if os.path.isfile("/usr/include/jpeglib.h"):
		extra_macros.append(('JPG_ENABLED', 1))
		jpg_libs = [ jpg_lib ]
	else:
		print "NO JPEG HEADERS FOUND"
		jpg_libs = []

#not ready yet. 
have_gmp = False # os.path.isfile("/usr/include/gmp.h")

# use currently specified compilers, not ones from when Python was compiled
# this is necessary for cross-compilation
compiler = os.environ.get("CC","gcc")
cxxcompiler = os.environ.get("CXX","g++")

fract4d_sources = [
	'fract4d/c/fract4dmodule.cpp',
	'fract4d/c/cmap.cpp',
	'fract4d/c/pointFunc.cpp',
	'fract4d/c/fractFunc.cpp',
	'fract4d/c/STFractWorker.cpp',
	'fract4d/c/MTFractWorker.cpp',
	'fract4d/c/image.cpp',
	'fract4d/c/imageIO.cpp',
	'fract4d/c/fract_stdlib.cpp'
	]

# this is a hack to build 2 versions of the same extension.
# we want to create a standard fract4dc which doesn't depend on gmp
# and a second fract4dcgmp which does. These are both built from the
# same source files but in the latter case with USE_GMP defined
# This is so I can ship a single binary for gnofract4d which supports
# both users with GMP and users without it, by conditionally loading
# the appropriate extension
fract4d_gmp_sources = []
if(have_gmp):
	for sourcefile in fract4d_sources:
		# this particular part of the hack is so that each file gets
		# compiled twice
		gmp_sourcefile = sourcefile.replace(".cpp","_gmp.cpp")
		os.system("cp %s %s" % (sourcefile, gmp_sourcefile))
		fract4d_gmp_sources.append(gmp_sourcefile)

module_gmp = Extension(
	'fract4d.gmpy',
	sources = [
	'fract4d/gmpy/gmpy.c'
	],
	libraries = ['gmp']
	)

defines = [ ('_REENTRANT',1),
			('THREADS',1),
			#('STATIC_CALC',1),
			#('NO_CALC', 1),  # set this to not calculate the fractal
			#('DEBUG_CREATION',1), # debug spew for allocation of objects
			#('DEBUG_ALLOCATION',1), # debug spew for array handling
			]

module_fract4dgmp = Extension(
	'fract4d.fract4dcgmp',
	sources = fract4d_gmp_sources,
	include_dirs = [
	'fract4d/c'
	],
	libraries = [
	'stdc++', 'gmp'
	] + jpg_libs,
	extra_compile_args = [
	'-Wall', '-Wno-strict-prototypes'
	] + png_flags,
	extra_link_args = png_libs, 
	define_macros = defines + [('USE_GMP',1)] + extra_macros,
	undef_macros = [ 'NDEBUG']	
	)

if 'win' == sys.platform[:3]:
	warnings = '/W3'
	libs = [ 'pthreadVC2', 'libdl', 'ws2_32' ]
	osdep = [ '/DWIN32', '/DWINDOWS', '/D_USE_MATH_DEFINES', '/D_CRT_SECURE_NO_WARNINGS', '/EHsc', '/Ox' ]
	extra_include = [ 'P:/x86/GnuWin32/include' ]
	includes = os.environ['GTK+_Include'].split(";")
	extra_include += [i for i in includes]
	extra_source = [ 'fract4d/c/win32func.cpp', 'fract4d/c/fract4d_stdlib_exports.cpp' ]
	if "64 bit" in sys.version:
		extra_link = [ 'P:/x86_64/GTK+/lib', 'P:/x86_64/GnuWin64/lib' ]
	else:
		extra_link = [ 'P:/x86/GTK+/lib', 'P:/x86/GnuWin32/lib' ]
	icon = ('share/pixmaps', ['pixmaps/gnofract4d-logo.ico'])
else:
	warnings = '-Wall'
	libs = [ 'stdc++' ]
	osdep = []
	include_dir = []
	extra_source = []
	extra_link = []
	extra_include = []
	icon = ('share/pixmaps', ['pixmaps/gnofract4d-logo.png'])

fract4d_sources += extra_source

module_fract4dc = Extension(
	'fract4d.fract4dc',
	sources = fract4d_sources,
	include_dirs = [
	'fract4d/c'
	] + extra_include,
	libraries = libs + jpg_libs,
	#library_dirs=['/home/edwin/gnofract4d'],
	extra_compile_args = [
	warnings,
	] + osdep + png_flags,
	library_dirs = extra_link,
	extra_link_args = png_libs,
	define_macros = defines + extra_macros,
	#undef_macros = [ 'NDEBUG'],
	)

module_cmap = Extension(
	'fract4d.fract4d_stdlib',
	sources = [
	'fract4d/c/cmap.cpp',
	'fract4d/c/image.cpp',
	'fract4d/c/fract_stdlib.cpp'
	] + extra_source,
	include_dirs = [
	'fract4d/c'
	] + extra_include,
	extra_compile_args = osdep,
	libraries = libs,
	library_dirs = extra_link,
	define_macros = [ ('_REENTRANT', 1)]
	)

modules = [module_fract4dc, module_cmap]
if have_gmp:
	modules.append(module_fract4dgmp)
	modules.append(module_gmp)
	
def get_files(dir,ext):
	return [ os.path.join(dir,x) for x in os.listdir(dir) if x.endswith(ext)] 

setup (name = 'gnofract4d',
	   version = gnofract4d_version,
	   description = 'A program to draw fractals',
	   long_description = \
'''Gnofract 4D is a fractal browser. It can generate many different fractals, 
including some which are hybrids between the Mandelbrot and Julia sets,
and includes a Fractint-compatible parser for your own fractal formulas.''',
	   author = 'Tim Whidbey',
	   author_email = 'catenary@users.sourceforge.net',
	   maintainer = 'Rachel Mant',
	   maintainer_email = 'dx-mon@users.sourceforge.net',
	   keywords = "fractal Mandelbrot Julia fractint chaos",
	   url = 'http://gnofract4d.sourceforge.net/',
	   packages = ['fract4d', 'fract4dgui', 'fractutils'], 
	   package_data = { 'fract4dgui' : [ 'ui.xml'] },
	   ext_modules = modules,
	   scripts = ['gnofract4d'],
	   data_files = [
		   # color maps
		   ('share/gnofract4d/maps',
			get_files("maps",".map") +
			get_files("maps",".cs") +
			get_files("maps", ".ugr")),

		   # formulas
		   ('share/gnofract4d/formulas',
			get_files("formulas","frm") +
			get_files("formulas", "ucl") +
			get_files("formulas", "uxf")),

		   # documentation
		   ('share/gnome/help/gnofract4d/C',
			get_files("doc/gnofract4d-manual/C", "xml")),
		   ('share/gnome/help/gnofract4d/C/figures',
			get_files("doc/gnofract4d-manual/C/figures",".png")),
		   ('share/gnome/help/gnofract4d/C',
			get_files("doc/gnofract4d-manual/C", "html")),
		   ('share/gnome/help/gnofract4d/C',
			get_files("doc/gnofract4d-manual/C",".css")),
		   
		   #internal pixmaps
		   ('share/pixmaps/gnofract4d',
			['pixmaps/improve_now.png',
			 'pixmaps/explorer_mode.png']),

           icon,
		   
		   # .desktop file
		   ('share/applications', ['gnofract4d.desktop']),

		   # MIME type registration
		   ('share/mime/packages', ['gnofract4d-mime.xml']),
		   
		   # doc files
		   ('share/doc/gnofract4d',
			['COPYING', 'README']),
		   ],
	   cmdclass={
		   "my_bdist_rpm": my_bdist_rpm.my_bdist_rpm,
		   "build" : my_build.my_build,
		   "my_build_ext" : my_build_ext.my_build_ext,
		   "install_lib" : my_install_lib.my_install_lib,
		   "install_egg_info" : my_install_egg_info.my_install_egg_info
		   }
	   )

# I need to find the file I just built and copy it up out of the build
# location so it's possible to run without installing. Can't find a good
# way to extract the actual target directory out of distutils, hence
# this egregious hack

so_extension = distutils.sysconfig.get_config_var("SO")

lib_targets = {
	"fract4dc" + so_extension : "fract4d",
	"fract4d_stdlib" + so_extension : "fract4d",
	"fract4dcgmp" + so_extension : "fract4d",
	"gmpy" + so_extension: "fract4d"
	}
if 'win' == sys.platform[:3]:
	lib_targets["fract4d_stdlib.lib"] = "fract4d"

def copy_libs(dummy,dirpath,namelist):
	 for name in namelist:
		 target = lib_targets.get(name)
		 if target != None:
			 name = os.path.join(dirpath, name)
			 shutil.copy(name, target)
			
os.path.walk("build",copy_libs,None)
if 'win' == sys.platform[:3]:
	shutil.copy("fract4d/fract4d_stdlib.pyd", "fract4d_stdlib.pyd")
