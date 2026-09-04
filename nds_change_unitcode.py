import sys
import os

header_pos_setting_unitcode = 0x12

def write_u32(data, pos, value):
	data[pos] = value & 0xFF
	data[pos + 1] = (value >> 8) & 0xFF
	data[pos + 2] = (value >> 16) & 0xFF
	data[pos + 3] = (value >> 24) & 0xFF

def main(argv):
	if len(argv) < 3:
		print("Usage: path setting_unitcode")
		return

	data = []
	with open(argv[1], "rb") as f:
		data = list(f.read())

	data[header_pos_setting_unitcode] = int(argv[2])

	with open(argv[1], "wb") as f:
		f.write(bytes(data))

if __name__ == '__main__':
	main(sys.argv)
