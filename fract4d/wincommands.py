import os

def getstatusoutput(cmd):
	pipe = os.popen(cmd, 'r')
	text = pipe.read()
	sts = pipe.close()
	if sts is None: sts = 0
	if text[-1:] == '\n': text = text[:-1]
	return sts, text