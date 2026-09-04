import sys
import os

header_pos_setting_title = 0
header_title_size = 0x0C

header_pos_setting_code_1 = header_title_size
header_pos_setting_code_2 = 0x230
header_code_size = 0x04

header_pos_setting_maker = header_title_size + header_code_size
header_maker_size = 0x02

def write_string(data, pos, value, size):
	for i in range(size):
		to_write = 0
		if len(value) > i:
			to_write = bytes(value[i], 'ascii')[0]
		data[pos + i] = to_write & 0xFF

def main(argv):
	if len(argv) < 2:
		print("Usage: path")
		return

	data = []
	with open(argv[1], "rb") as f:
		data = list(f.read())

	write_string(data, header_pos_setting_title, "HOMEBREW", header_title_size)
	write_string(data, header_pos_setting_code_1, "####", header_code_size)
	write_string(data, header_pos_setting_code_2, "####", header_code_size)
	write_string(data, header_pos_setting_maker, "", header_maker_size)

	with open(argv[1], "wb") as f:
		f.write(bytes(data))

if __name__ == '__main__':
	main(sys.argv)
