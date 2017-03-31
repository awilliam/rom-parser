rom-parser/rom-fixer
====================

$ git clone https://github.com/awilliam/rom-parser
$ cd rom-parser
$ make

To view ROM contents:

usage: rom-parser [ROM file]

This program does not have support for reading the ROM from pci-sysfs, please do this manually in advance, ex:

# cd /sys/bus/pci/devices/0000:01:00.0/
# echo 1 > rom
# cat rom > /tmp/image.rom
# echo 0 > rom

Pass the resulting image file as the argument to this program.

To modify ROM conents:

usage: rom-fixer [ROM file]

Obtain ROM as above, program prompts for modifying ROM vendor and device IDs and invalid checksums.

IMPORTANT: rom-fixer will update the ROM file in place.  Make a backup!
