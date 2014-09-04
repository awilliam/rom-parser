#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

struct __attribute__ ((__packed__)) header {
	unsigned char sig1;
	unsigned char sig2;
	unsigned short init_size;
	unsigned int header_sig;
	unsigned short efi_subsys;
	unsigned short efi_machine;
	unsigned char resv[10];
	unsigned short efi_offset;
	unsigned short pcir_offset;
};

struct __attribute__ ((__packed__)) pcir {
	char sig[4];
	unsigned short vendor;
	unsigned short device;
	unsigned short device_list;
	unsigned short pcir_length;
	unsigned char pcir_rev;
	unsigned char class[3];
	unsigned short image_length;
	unsigned short rom_rev;
	unsigned char type;
	unsigned char last;
	unsigned short runtime_length;
	unsigned short config_header;
	unsigned short dmtf_entry;
};

int usage(char *name)
{
	printf("usage: %s [ROM image file]\n", name);
	return 0;
}

int main(int argc, char **argv)
{
	struct stat st;
	int fd, offset = 0, ret = 0;
	void *map;
	struct header *header;
	struct pcir *pcir;

	if (argc != 2)
		return usage(argv[0]);

	if (stat(argv[1], &st)) {
		printf("Unable to stat ROM file: %m\n");
		return -1;
	}

	if (!st.st_size) {
		printf("Error, zero sized ROM file\n");
		return -1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		printf("Unable to open ROM file: %m\n");
		return -1;
	}

	map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED) {
		printf("Unable to mmap ROM file: %m\n");
		ret = -1;
		goto out_close;
	}

next:
	if (offset >= st.st_size) {
		printf("Error, ran off the end\n");
		ret = -1;
		goto out_unmap;
	}

	header = map + offset;

	if (header->sig1 != 0x55 || header->sig2 != 0xaa) {
		offset += 512;
		goto next;
	}

	printf("Valid ROM signature found @%xh, PCIR offset %xh\n",
	       offset, header->pcir_offset);
	pcir = map + offset + header->pcir_offset;

	if (strncmp(pcir->sig, "PCIR", 4)) {
		printf("Invalid PCIR signature: %c%c%c%c\n", 
		       pcir->sig[0], pcir->sig[1], pcir->sig[2], pcir->sig[3]);
		ret = -1;
		goto out_unmap;
	}

	printf("\tPCIR: type %x, vendor: %04x, device: %04x, "
	       "class: %02x%02x%02x\n", pcir->type, pcir->vendor, pcir->device,
	       pcir->class[2], pcir->class[1], pcir->class[0]);
	printf("\tPCIR: revision %x, vendor revision: %x\n",
	       pcir->pcir_rev, pcir->rom_rev);

	if (pcir->type == 3) {
		printf("\t\tEFI: Signature %sValid\n",
		       header->header_sig != 0x0ef1 ? "NOT " : "");

	}

	if (!pcir->last) {
		offset += 512;
		goto next;
	}

	printf("\tLast image\n");

out_unmap:
	munmap(map, st.st_size);
out_close:
	close(fd);
	return ret;
}
