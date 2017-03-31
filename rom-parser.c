#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

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
	int i, fd, offset = 0, ret = 0;
	void *map;
	struct header *header;
	struct pcir *pcir;
	char answer;
	unsigned char csum, *data;

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

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Unable to open ROM file: %m\n");
		return -1;
	}

	map = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
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

	printf("\tPCIR: type %x%s, vendor: %04x, device: %04x, "
	       "class: %02x%02x%02x\n", pcir->type,
	       pcir->type == 0 ? " (x86 PC-AT)" : pcir->type == 3 ?
	       " (EFI)" : "", pcir->vendor, pcir->device,
	       pcir->class[2], pcir->class[1], pcir->class[0]);
	printf("\tPCIR: revision %x, vendor revision: %x\n",
	       pcir->pcir_rev, pcir->rom_rev);

	if (pcir->type == 3) {
		int valid = header->header_sig == 0x0ef1;
		printf("\t\tEFI: Signature %sValid",
		       !valid ? "NOT " : "");
		if (valid) {
			printf(", Subsystem: ");
			switch (header->efi_subsys) {
			case 0xb:
				printf("Boot");
				break;
			case 0xc:
				printf("Runtime");
				break;
			default:
				printf("Unknown (%x)", header->efi_subsys);
			}
			printf(", Machine: ");

			switch (header->efi_machine) {
			case 0x014c:
				printf("IA-32");
				break;
			case 0x0200:
				printf("Itanium");
				break;
			case 0x0EBC:
				printf("EFI Byte Code");
				break;
			case 0x8664:
				printf("X64");
				break;
			case 0x01c2:
				printf("ARM");
				break;
			case 0xaa64:
				printf("ARM 64-bit");
				break;
			default:
				printf("Unknown (%x)", header->efi_machine);
			}
		}
		printf("\n");
	}

#ifdef FIXER
ask_vendor:
	printf("\nModify vendor ID %04x? (y/n): ", pcir->vendor);
	answer = getchar();
	getchar();
	switch (tolower(answer)) {
	case 'n':
		break;
	case 'y':
	{
		unsigned short vendor;

		printf("New vendor ID: ");
		ret = scanf("%hx", &vendor);
		getchar();
		if (ret == 1) {
			printf("Overwrite vendor ID with %04x? (y/n): ", vendor);
			answer = getchar();
			getchar();
			if (tolower(answer) == 'y') {
				pcir->vendor = vendor;
				break;
			}
		}
	}
	default:
		printf("Unrecognized response, try again\n");
		goto ask_vendor;
	}

ask_device:
	printf("Modify device ID %04x? (y/n): ", pcir->device);
	answer = getchar();
	getchar();
	switch (tolower(answer)) {
	case 'n':
		break;
	case 'y':
	{
		unsigned short device;

		printf("New device ID: ");
		ret = scanf("%hx", &device);
		getchar();
		if (ret == 1) {
			printf("Overwrite device ID with %04x? (y/n): ", device);
			answer = getchar();
			getchar();
			if (tolower(answer) == 'y') {
				pcir->device = device;
				break;
			}
		}
	}
	default:
		printf("Unrecognized response, try again\n");
		goto ask_device;
	}
#endif

	if (!pcir->last) {
		offset += 512;
		goto next;
	}

	printf("\tLast image\n");

#ifdef FIXER
	data = map;
	for (csum = 0, i = 0; i < st.st_size; i++)
		csum += data[i];

	if (csum) {
ask_csum:
		printf("ROM checksum is invalid, fix? (y/n): ");
		answer = getchar();
		getchar();
		switch (tolower(answer)) {
		case 'n':
			break;
		case 'y':
			data[6] = -(csum - data[6]);
			break;
		default:
			printf("Unrecognized response, try again\n");
			goto ask_csum;
		}
	} else
		printf("ROM checksum OK\n");
#endif

out_unmap:
	munmap(map, st.st_size);
out_close:
	close(fd);
	return ret;
}
