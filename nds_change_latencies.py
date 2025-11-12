import sys
import os

header_pos_setting_normal = 0x60
header_pos_setting_key1 = 0x64
header_pos_setting_sec_area_delay = 0x6E

def write_u16(data, pos, value):
	data[pos] = value & 0xFF
	data[pos + 1] = (value >> 8) & 0xFF

def write_u32(data, pos, value):
	data[pos] = value & 0xFF
	data[pos + 1] = (value >> 8) & 0xFF
	data[pos + 2] = (value >> 16) & 0xFF
	data[pos + 3] = (value >> 24) & 0xFF

def main(argv):
	if len(argv) < 5:
		print("Usage: path setting_normal_commands_hex setting_key1_commands_hex secure_area_delay_hex")
		return

	data = []
	with open(argv[1], "rb") as f:
		data = list(f.read())

	write_u32(data, header_pos_setting_normal, int(argv[2], 16))
	write_u32(data, header_pos_setting_key1, int(argv[3], 16))
	write_u16(data, header_pos_setting_sec_area_delay, int(argv[4], 16))

	with open(argv[1], "wb") as f:
		f.write(bytes(data))

if __name__ == '__main__':
	main(sys.argv)
