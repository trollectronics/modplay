#include <stdint.h>
#include <stdio.h>
#include <sys/file.h>
#include <draw/font.h>
#include <font.h>
#include <terminal.h>
#include <interrupt.h>
#include <muil/muil.h>
#include <sd.h>
#include <fat.h>
#include "main.h"

uint8_t fat_buf[512];

DrawFont *font_small;

static const char *sd_card_type_name[] = {
	[SD_CARD_TYPE_MMC] = "MMC",
	[SD_CARD_TYPE_SD] = "SD",
	[SD_CARD_TYPE_SDHC] = "SDHC",
};

static const char *fat_type_name[] = {
	[FAT_TYPE_FAT16] = "FAT16",
	[FAT_TYPE_FAT32] = "FAT32",
};

static int fat_read_sd(uint32_t sector, uint8_t *data) {
	SDStreamStatus status;
	
	status = SD_STREAM_STATUS_BEGIN;
	
	sd_stream_read_block(&status, sector);
	if(status == SD_STREAM_STATUS_FAILED) {
		printf("sd_stream_read_block failed at SD_STREAM_STATUS_BEGIN\n");
		return -1;
	}
	
	while(status >= 1)
		*data++ = sd_stream_read_block(&status);
	
	if(status == SD_STREAM_STATUS_FAILED) {
		printf("sd_stream_read_block failed\n");
		return -1;
	}
	
	return 0;
}

static int fat_write_sd(uint32_t sector, uint8_t *data) {
	SDStreamStatus status;
	
	status = SD_STREAM_STATUS_BEGIN;
	
	sd_stream_write_block(&status, sector);
	if(status == SD_STREAM_STATUS_FAILED)
		return -1;
	
	while(status >= 1)
		sd_stream_write_block(&status, *data++);
	
	if(status == SD_STREAM_STATUS_FAILED)
		return -1;
	
	return 0;
}

static void print_filesize(uint32_t filesize) {
	if(filesize < 1024)
		printf("%u", filesize);
	else if(filesize < 1024*1024)
		printf("%uk", filesize/1024U);
	else
		printf("%uM", filesize/(1024U*1024U));
}

size_t write_terminal(const void *ptr, size_t size, File *f) {
	size_t i;
	char *s = (char *) ptr;
	
	for(i = 0; i < size; i++)
		terminal_putc_term(*s++);
	
	return size;
}

FileHandler fh_terminal = {
	.open = NULL,
	.close = NULL,
	.read = NULL,
	.write = write_terminal,
};

File file_terminal = {
	.handler = &fh_terminal,
};

int main(int argc, char **argv) {
	int type;
	char label[12];
	
	interrupt_init();
	
	terminal_init();
	terminal_clear();
	
	stdin = (FILE *) &file_terminal;
	stdout = (FILE *) &file_terminal;
	stderr = (FILE *) &file_terminal;
	
	printf("Detecting SD card: ");
	if((type = sd_init()) == SD_CARD_TYPE_INVALID) {
		goto fail;
	}
	
	terminal_set_fg(TERMINAL_COLOR_LIGHT_GREEN);
	printf("%s\n", sd_card_type_name[type]);
	terminal_set_fg(TERMINAL_COLOR_WHITE);
	printf(" - Card size: ");
	print_filesize(sd_get_card_size()/2*1024);
	printf("B\n");
	
	printf("Detecting file system: ");
	if(fat_init(fat_read_sd, fat_write_sd, fat_buf) < 0) {
		goto fail;
	}
	
	type = fat_type();
	terminal_set_fg(TERMINAL_COLOR_LIGHT_GREEN);
	printf("%s\n", fat_type_name[type]);
	terminal_set_fg(TERMINAL_COLOR_WHITE);
	fat_get_label(label);
	printf(" - Volume label: %s\n\n", label);
	
	font_small = draw_font_new(vgafont_data, 8, 16);
	muil_init(4, font_small);
	
	player_init();
	
	terminal_set_bg(TERMINAL_COLOR_CYAN);
	terminal_clear();
	terminal_set_bg(TERMINAL_COLOR_BLACK);
	
	browse();
	for(;;);
	
	fail:
	terminal_set_fg(TERMINAL_COLOR_LIGHT_RED);
	printf("failed");
	terminal_set_fg(TERMINAL_COLOR_WHITE);
	for(;;);
	
	return 0;
}

