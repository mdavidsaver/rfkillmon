#!/usr/bin/env python
"""
Monitor event stream from Linux RF Kill subsystem

See kernel source Documentation/rfkill.txt
"""

import struct, errno

S = struct.Struct('=IBBBB')
assert S.size==8

ETYPES = {
	0:'All',
	1:'WLAN',
	2:'Bluetooth',
	3:'UWB',
	4:'WiMax',
	5:'WWAN',
	6:'GPS',
	7:'FM',
}

OPS = {
	0:'Add',
	1:'Del',
	2:'Change',
	3:'Change All',
}

def getname(idx):
	try:
		with open('/sys/class/rfkill/rfkill%d/name'%idx,'r') as F:
			return F.readline().strip()
	except IOError as E:
		if E.errno in (errno.ENOENT, errno.EPERM):
			return '<%d>'%idx
		raise

def loop():
	F = open('/dev/rfkill','r')

	while True:
		evt = F.read(S.size)
		if not evt:
			print 'Done?'
			return
		idx, etype, op, hard, soft = S.unpack(evt)

		tname = ETYPES.get(etype, '<%d>'%etype)
		opname = OPS.get(op, '<%d>'%op)

		print 'Event for',getname(idx)
		print ' Type:',tname
		print ' Op  :',opname
		print ' Hard:',hard
		print ' Soft:',soft

if __name__=='__main__':
	try:
		loop()
	except KeyboardInterrupt:
		print 'Done'
