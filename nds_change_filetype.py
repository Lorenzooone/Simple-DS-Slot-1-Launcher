import sys
import os

header_pos_setting_filetype = 0x234

def write_u32(data, pos, value):
	data[pos] = value & 0xFF
	data[pos + 1] = (value >> 8) & 0xFF
	data[pos + 2] = (value >> 16) & 0xFF
	data[pos + 3] = (value >> 24) & 0xFF

def main(argv):
	if len(argv) < 3:
		print("Usage: path setting_filetype")
		return

	data = []
	with open(argv[1], "rb") as f:
		data = list(f.read())

	write_u32(data, header_pos_setting_filetype, int(argv[2], 16))

	with open(argv[1], "wb") as f:
		f.write(bytes(data))

if __name__ == '__main__':
	main(sys.argv)
