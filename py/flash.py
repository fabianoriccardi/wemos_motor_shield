#!/usr/bin/python
import time
import os
import sys
import getopt
import filecmp
from wemosh import *


def usage():
	print "Usage\r\n\
	-h, --help \t\t\tprint usage info\r\n\
	-a, --address=<I2C addres> \tI2C slave address\r\n\
	-r, --read=<file> \t\tread FW from flash\r\n\
	-w, --write=<file> \t\twrite FW to flash\r\n\
	-n, --noverify \t\t\tskip written FW verification"


def main():
	address = 0x30
	out_file = None
	in_file = None
	verify = True

	try:
		opts, args = getopt.getopt(sys.argv[1:], "ha:r:w:n", \
			["help", "address=", "read=", "write=", "noverify="])
	except getopt.GetoptError:
		usage()
		sys.exit(1)

	for opt, arg in opts:
		if opt in ("-h", "--help"):
			usage()
			sys.exit()
		if opt in ("-d", "--address"):
			try:
				address = int(arg, 16)
			except ValueError:
				print "Wrong address argument:", arg
				usage()
				sys.exit(1)
		elif opt in ("-r", "--read"):
			out_file = arg
		elif opt in ("-w", "--write"):
			if not os.path.isfile(arg):
				print "File not found:", arg
				sys.exit(1)
			in_file = arg
		elif opt in ("-v", "--verify"):
			verify = False


	shield = WMShield(address)

	if not shield.detect():
		print "No shield (0x%x) found" %(address)
		sys.exit(2)
	else:
		print "Shield found (0x%x)" %(address)

	if shield.get_id() != IAP:
		print "Starting bootloader"
		if not shield.boot(IAP):
			print "Bootloader start error, ID: 0x%X" %(shield.get_id())
		else: 
			print "OK"

	if in_file != None:
		print "Writing FW"
		if not shield.write_fw(in_file, verify):
			print "FW write failed"
			sys.exit()

		print "Starting main FW"
		if not shield.boot(FW):
			print "Main FW start error, ID: 0x%X" %(shield.get_id())
			sys.exit()

	elif out_file != None:
		size = shield.get_fwlen()
		if not size:
			print "FW size get failed"
			sys.exit(2)
		print "Reading FW (%d bytes)" % (size)

		if len(shield.read_fw(size, out_file)) == 0:
			print "FW read failed"
			sys.exit(2)

	print "Done"


if __name__ == '__main__':
	main()