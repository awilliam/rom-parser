rom-parser
==========

$ git clone https://github.com/awilliam/rom-parser
$ cd rom-parser
$ make

usage: rom-parser [ROM file]

This program does not have support for reading the ROM from pci-sysfs, please do this manually in advance, ex:

# cd /sys/bus/pci/devices/0000:01:00.0/
# echo 1 > rom
# cat rom > /tmp/image.rom
# echo 0 > rom

Pass the resulting image file as the argument to this program.
