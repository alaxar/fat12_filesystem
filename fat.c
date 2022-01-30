#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef uint8_t bool;
#define false 0
#define true 1

typedef struct bootRecord{
	uint8_t BootJumpInstrution[3];
	uint8_t OEM_Identifier[8];
	uint16_t BytesPerSector;
	uint8_t NoSecPerCluster;
	uint16_t NoReservedSec;
	uint8_t NoFATTables;
	uint16_t NoRootDirectories;
	uint16_t TotalNoSectors;
	uint8_t MediaTypeDesc;
	uint16_t NumberofSectorsPerFat;
	uint16_t NumberofSectorsPerTrack;
	uint16_t NumberOfHeader;
	uint32_t NumberOfHiddenSec;
	uint32_t LargeSectorCount;

	uint8_t drive_number;	
	uint8_t Reserved;
	uint8_t Signature;
	uint32_t Volume_id;	
	uint8_t Volume_label[11];
	uint8_t System_identifier[8];
} __attribute__((packed)) bootRecord;

typedef struct DirectoryEntry {
	uint8_t filename[11];
	uint8_t attributes;
	uint8_t reserved;
	uint8_t CreationTime;
	uint16_t CreatedTime;
	uint16_t CreatedDate;
	uint16_t LastAccessed;
	uint16_t HighFirstCluster;		// this is always zero for fat12 and fat16
	uint16_t LastModifiedTime;
	uint16_t LastModifiedDate;
	uint16_t LowFirstCluster;		// use this number to find the first cluster of this entry(file)
	uint32_t FileSize;			// size of the file in bytes
} __attribute__((packed)) DirectoryEntry;

struct bootRecord boot;
struct DirectoryEntry Dir[224];
uint32_t *g_fat = NULL;

void help() {
	printf("list - to list files in a disk\n");
}
// reading the bootsector
bool ReadingBootSector(FILE *disk) {
	return fread(&boot, 1, sizeof(struct bootRecord), disk);
}

bool readFile(DirectoryEntry* fileEntry, FILE* disk, uint8_t* outputBuffer)
{
	// the data is found at (bootsector + (SecPerFat * BytesPerSector * NumberOfFats) + (DirectoryEntrySize * 32 bytes)
	int lba = boot.BytesPerSector + (boot.NumberofSectorsPerFat * boot.BytesPerSector * boot.NoFATTables) + (boot.NoRootDirectories * 32);
	printf("LBA: %d\n", lba);
	
	fseek(disk, lba, SEEK_SET);

	fread(outputBuffer, 1, 13, disk);	
	return true;
}

bool ReadingDirectoryEntry(FILE *disk) {
	// the directory entry is right after the 2 fats
	int DirectoryEntryOffset = (boot.NoReservedSec * 512) + boot.NumberofSectorsPerFat * boot.BytesPerSector * boot.NoFATTables; // (1 * 512) + 9 * 512 * 2
	// seeking to the offset
	fseek(disk, DirectoryEntryOffset, SEEK_SET);
	if(fread(&Dir, 1, (sizeof(struct DirectoryEntry) * boot.NoRootDirectories), disk) > 0)
		return true;
}

DirectoryEntry *findFile(char *filename) {
	for(int i = 0; i < boot.NoRootDirectories; i++) {
		if(memcmp(Dir[i].filename, filename, 11) == 0) {
			return &Dir[i];
		}
	}
	return NULL;
}

void listfiles() {
	printf(" Volume in Drive: %s\n", Dir[0].filename);
	printf(" Volume Serial Number is %x\n", boot.Volume_id);
	
	printf("Directory for ::/\n\n");
	for(int i = 1, j = 0; (Dir[i].FileSize != 0 && strlen(Dir[i].filename) != 0); i++, j++) {
		printf("%s\t\t%d bytes\n\n", Dir[i].filename, Dir[i].FileSize);
	}
}

int main(int argc, char *argv[]) {
	FILE *diskRead = fopen("test.img", "rb");
	if(argc < 2) {
		printf("Usage: %s <test.img> <filename>\n", argv[0]);
		printf("Or: %s <test.img> list\n", argv[0]);
		return -1;
	}
	if(!diskRead) {
		perror("Disk Read\n");
		return -1;
	}
	
	// reading the boot sector	
	if(!ReadingBootSector(diskRead)) {
		fprintf(stderr, "Error reading boot sector\n");
		return -1;
	}
	
	if(!ReadingDirectoryEntry(diskRead)) {
		fprintf(stderr, "Error reading Directory Entry\n");
		return -2;
	}
	
	

	if(!ReadingDirectoryEntry(diskRead)) {
		fprintf(stderr, "Error while reading directory entries\n");
		return -3;
	}

	if(strcmp(argv[2], "list") == 0) {
		listfiles();
		return 0;
	}
	
	DirectoryEntry *findfile = findFile(argv[2]);
	if(!findfile) {
		fprintf(stderr, "Can not find file\n");
		return -4;
	}
	
	unsigned char *buffer = (uint8_t*)malloc(findfile->FileSize + boot.BytesPerSector);
	if(!readFile(findfile, diskRead, buffer)) {
	        fprintf(stderr, "Could not read file %s!\n", argv[2]);
	        return -5;
	}

	printf("%s\n", buffer);
	return 0;
}
