#!/usr/bin/python

from distutils.core import Command
from distutils.spawn import spawn

import os

class my_build_mo(Command):
	description  = "Compiles .po files into .mo files"

	def initialize_options(self):
		pass

	def finalize_options(self):
		pass

	def __compile_mo(self, src, dest):
		print "Compiling %s" % src
		spawn(['msgfmt', src, '--output-file', dest])

	def run(self):
		poPath = os.path.join(os.path.dirname(os.curdir), 'po')
		for (path, names, filenames) in os.walk(poPath):
			for file in filenames:
				if (file.endswith('.po')):
					lang = file[:-3]
					src = os.path.join(path, file)
					destPath = os.path.join('build', 'locale', lang, 'LC_MESSAGES')
					dest = os.path.join(destPath, 'gnofract4d.mo')
					if (not os.path.exists(destPath)):
						os.makedirs(destPath)
					if (not os.path.exists(dest)):
						self.__compile_mo(src, dest)
					else:
						srcMtime = os.stat(src)[8]
						destMtime = os.stat(dest)[8]
						if (srcMtime > dstMtime):
							self.__compile_mo(src, dest)
