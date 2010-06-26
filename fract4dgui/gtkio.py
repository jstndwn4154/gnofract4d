import sys, os

try:
	from fract4d import fract4dcgmp as fract4dc
except ImportError, err:
    from fract4d import fract4dc

# stub class for selecting a suitable readwrite method depending on platform, to be used whenever a file desciptor returned from fract4dc.pipe() is used.
class gtkio:
	def __init__(self):
		if 'win' == sys.platform[:3]:
			self.__onWindows = True
		else:
			self.__onWindows = False

	def pipe(self):
		if self.__onWindows == True:
			return fract4dc.pipe()
		else:
			return os.pipe()

	def read(self, fd, len):
		if self.__onWindows == True:
			#return None
			return fract4dc.read(fd, len)
		else:
			return os.read(fd, len)

	def write(self, fd, len):
		if self.__onWindows == True:
			return fract4dc.write(fd, len)
		else:
			return os.write(fd, len)
