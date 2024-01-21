#!/usr/bin/env python3

# objdump --disassembly=<symbol> -S -M intel <path>

import os
import sys

def cmd(command):
	return os.system(command)

# bash style -xe echo
def cmd_echoed(command):
	print("+ " + command)
	return cmd(command)


def objdump(input_path, output_path, symbol, options):
	command = "objdump --disassemble={} {} {} >> {}".format(symbol, options, input_path, output_path)
	cmd_echoed(command)

def usage():
	print("Usage; {} [symbol]".format(sys.argv[0]))

def main():
	input_path = "mix"
	output_path = "dump.txt"
	if len(sys.argv) <= 1:
		usage()
		return
	cmd("</dev/null> {}".format(output_path))
	for i in range(1, len(sys.argv)):
		objdump(input_path, output_path, sys.argv[i], "-S -M intel -j .text")

if __name__ == "__main__":
	main()
