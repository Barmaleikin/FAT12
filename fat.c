#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


struct boot_block {
	unsigned char boostrap_1[3];
	unsigned char manufacturer[8];
	unsigned short nb_bytes_per_block;
	unsigned char nb_blocks_per_alloc;
	unsigned short nb_reserved_block;
	unsigned char nb_fat;
	unsigned short nb_root_dirs;
	unsigned short nb_blocks_on_disk;
	unsigned char meta_desc;
	unsigned short nb_blocks_on_fat;
	unsigned short nb_blocks_per_track;
	unsigned short nb_heads;
	unsigned int nb_hidden_block;
	unsigned int nb_blocks_on_disk2;
	unsigned short physical_drive_nb;
	unsigned char sign;
	unsigned int serial_num;
	unsigned char label[11];
	unsigned char fs_id[8];
};

struct root_dir_entry {
	unsigned char filename[8];
	unsigned char extension[3];
	unsigned char attr;
	unsigned char reserved[10];
	unsigned short time_last_updated;
	unsigned short date_last_updated;
	unsigned short starting_cluster;
	unsigned int file_size;
};

void
read_boot_block(struct boot_block *bb, FILE **f)
{

	// go back to the start of the fs
	fseek(*f, 0, SEEK_SET);
	fread(bb->boostrap_1, sizeof(unsigned char), 3, *f);
	fread(bb->manufacturer, sizeof(unsigned char), 8, *f);
	fread(&bb->nb_bytes_per_block, sizeof(unsigned char), 2, *f);
	fread(&bb->nb_blocks_per_alloc, sizeof(unsigned char), 1, *f);
	fread(&bb->nb_reserved_block, sizeof(unsigned char), 2, *f);
	fread(&bb->nb_fat, sizeof(unsigned char), 1, *f);
	fread(&bb->nb_root_dirs, sizeof(unsigned char), 2, *f);
	fread(&bb->nb_blocks_on_disk, sizeof(unsigned char), 2, *f);
	fread(&bb->meta_desc, sizeof(unsigned char), 1, *f);
	fread(&bb->nb_blocks_on_fat, sizeof(unsigned char), 2, *f);
	fread(&bb->nb_blocks_per_track, sizeof(unsigned char), 2, *f);
	fread(&bb->nb_heads, sizeof(unsigned char), 2, *f);
	fread(&bb->nb_hidden_block, sizeof(unsigned char), 4, *f);
	fread(&bb->nb_blocks_on_disk2, sizeof(unsigned char), 4, *f);
	fread(&bb->physical_drive_nb, sizeof(unsigned char), 2, *f);
	fread(&bb->sign, sizeof(unsigned char), 1, *f);
	fread(&bb->serial_num, sizeof(unsigned char), 4, *f);
	fread(&bb->label, sizeof(unsigned char), 11, *f);
	fread(&bb->fs_id, sizeof(unsigned char), 8, *f);
}

void
read_root_directories(struct root_dir_entry *r, struct boot_block bb, FILE **f)
{
	long pos;
	unsigned int i;
	unsigned char bb_sign[2];

	// start of the dir entries: Boot block + FATs
	// every dir entry is 32B
	// we skip the first one as this is a special entry for the disk label
	pos = (bb.nb_blocks_on_fat*bb.nb_fat+1)*bb.nb_bytes_per_block + 0x20;
	fseek(*f, pos, SEEK_SET);

	for (i = 0; i < bb.nb_root_dirs; i++) {
		fread(r[i].filename, sizeof(unsigned char), 8, *f);
		fread(r[i].extension, sizeof(unsigned char), 3, *f);
		fread(&r[i].attr, sizeof(unsigned char), 1, *f);
		fread(r[i].reserved, sizeof(unsigned char), 10, *f);
		fread(&r[i].time_last_updated, sizeof(unsigned char), 2, *f);
		fread(&r[i].date_last_updated, sizeof(unsigned char), 2, *f);
		fread(&r[i].starting_cluster, sizeof(unsigned char), 2, *f);
		fread(&r[i].file_size, sizeof(unsigned char), 4, *f);
	}
}

void
print_f_attr(unsigned char attr)
{

	printf("\tattributes: ");
	if (attr & 0x01) {
		printf("readonly ");
	}
	if (attr & 0x02) {
		printf("hidden ");
	}
	if (attr & 0x04) {
		printf("system-file ");
	}
	if (attr & 0x10) {
		printf("subdirectory ");
	}
	if (attr & 0x20) {
		printf("archive ");
	}
	puts("");
}

void
print_f_time(unsigned short d)
{
	unsigned int hours = 0;
	unsigned int minutes = 0;
	unsigned int seconds = 0;
	unsigned char i;

	printf("\ttime: ");
        hours |= (d >> 11) & ( (1 << 5) -1);
	hours += 1;
        minutes |= (d >> 5) & ( (1 << 6) -1 );
	minutes += 1;
        seconds |= d & ( (1 << 5) -1 );
	seconds *= 2;
	seconds += 1;

	printf("%d:%d:%d\n", hours, minutes, seconds);
}

void
print_f_date(unsigned short d)
{
	unsigned int year = 0;
	unsigned int month = 0;
	unsigned int day = 0;
	unsigned char i;

	printf("\tdate: ");
	for (i = 0; i < 7; i++) {
		year |= (d >> 9) & (1 << i);
	}
	year += 1980;

	for (i = 0; i < 4; i++) {
		month |= (d >> 5) & (1 << i);
	}
	for (i = 0; i < 5; i++) {
		day |= d & (1 << i);
	}

	printf("%d/%d/%d\n", year, month, day);
}

void
print_root_directories(struct root_dir_entry *r, unsigned short nb_entries)
{
	int i;

	puts("root entries: {");
	for (i = 0; i < nb_entries; i++) {
		if (r[i].starting_cluster == 0) {
			// skip empty entries
			continue;
		}
		printf("\tfilename: %s\n", r[i].filename);
		printf("\textension: %s\n", r[i].extension);
		print_f_attr(r[i].attr);
		printf("\tfile size: %d Bytes\n", r[i].file_size);
		print_f_time(r[i].time_last_updated);
		print_f_date(r[i].date_last_updated);
		printf("\tstarting cluster: %d \n", r[i].starting_cluster);
		puts("");
	}
	puts("}");
}

void
print_boot_block(struct boot_block bb)
{

	puts("boot block info: {");
	printf("\tmanufacturer %s\n", bb.manufacturer);
	printf("\tbytes per block %d\n", bb.nb_bytes_per_block);
	printf("\blocks per alloc %d\n", bb.nb_blocks_per_alloc);
	printf("\tnb reserved block %d\n", bb.nb_reserved_block);
	printf("\tnb fat %d\n", bb.nb_fat);
	printf("\tnb root dirs %d\n", bb.nb_root_dirs);
	printf("\tnb blocks on disk %d\n", bb.nb_blocks_on_disk);
	printf("\tmeta desc %d\n", bb.meta_desc);
	printf("\tnb blocks on fat %d\n", bb.nb_blocks_on_fat);
	printf("\tnb blocks per track %d\n", bb.nb_blocks_per_track);
	printf("\tnb heads %d\n", bb.nb_heads);
	printf("\tnb hidden blocks %d\n", bb.nb_hidden_block);
	printf("\tnb blocks on disk 2 %d\n", bb.nb_blocks_on_disk2);
	printf("\tphysical drive nb %d\n", bb.physical_drive_nb);
	printf("\tsignature %d\n", bb.sign);
	printf("\tserial number %d\n", bb.serial_num);
	printf("\tlabel %s\n", bb.label);
	printf("\tfs_id %s\n", bb.fs_id);
	puts("}");
}

unsigned short
get_next_block(unsigned short block, struct boot_block bb, FILE **f)
{
	// this is a fat12 FS, we read 12 bits 1.5 Bytes
	long pos;
	unsigned short next_block;

	// go directly to the fat block
	pos = bb.nb_bytes_per_block + (long)(block)*1.5;
	fseek(*f, pos, SEEK_SET);
	fread(&next_block, sizeof(unsigned char), 2, *f);

	if (block % 2 == 0) {
		// set high 4 bits to zero
		next_block &= 0x000FFF;
	}
	else {
		next_block >>= 4; 
		next_block &= 0x000FFF;
	}

	return next_block;
}

unsigned char *
get_file_content(unsigned short block, struct boot_block bb, FILE **f)
{
	long pos;
	unsigned char *buffer;

	buffer = calloc(1, bb.nb_bytes_per_block*bb.nb_blocks_per_alloc);

	pos = (bb.nb_blocks_on_fat*bb.nb_fat+1)*bb.nb_bytes_per_block
		+ (0x20*bb.nb_root_dirs)
		+ (block-2)*(bb.nb_bytes_per_block*bb.nb_blocks_per_alloc);

	fseek(*f, pos, SEEK_SET);
	fread(
		buffer,
		sizeof(unsigned char),
		bb.nb_bytes_per_block*bb.nb_blocks_per_alloc,
		*f
	);

	return buffer;
}

void
print_file_content(struct root_dir_entry r, struct boot_block bb, FILE **f)
{
	unsigned short next_block;
	unsigned char* f_content;
	unsigned char* buffer;
	unsigned short pos;

	printf("\n\n-----\n%s:\n", r.filename);
	pos = 0;
	// safely add a block, just in case
	f_content = malloc(r.file_size
		+bb.nb_bytes_per_block*bb.nb_blocks_per_alloc);
	next_block = r.starting_cluster;
	while (next_block != 0xFFF) {
		buffer = get_file_content(next_block, bb, f);
		memcpy(
			f_content+pos,
			buffer,
			bb.nb_bytes_per_block*bb.nb_blocks_per_alloc
		);
		next_block = get_next_block(next_block, bb, f);
		pos += bb.nb_bytes_per_block*bb.nb_blocks_per_alloc;
		free(buffer);
	}

	printf("%s\n", f_content);

	free(f_content);
}

int
main(int argc, char** argv)
{
	struct boot_block bb;
	FILE *f;
	long pos;
	unsigned char bb_sign[2];
	struct root_dir_entry *r_entries;

	if (argc < 2) {
		puts("Pass the image name");
		return 1;
	}

	f = fopen(argv[1], "r");
	if (f == NULL) {
		perror(NULL);
		return 1;
	}

	memset(&bb, 0, sizeof(struct boot_block));
	read_boot_block(&bb, &f);
	print_boot_block(bb);

	r_entries = calloc(
		sizeof(struct root_dir_entry),
		bb.nb_root_dirs
	);
	read_root_directories(r_entries, bb, &f);
	puts("");
	print_root_directories(r_entries, bb.nb_root_dirs);

	for (pos = 0; pos < bb.nb_root_dirs; pos++) {
		if (r_entries[pos].starting_cluster == 0) {
			continue;
		}
		print_file_content(r_entries[pos], bb, &f);
	}

	free(r_entries);
	return 0;
}

