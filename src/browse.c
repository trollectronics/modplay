#include <stdio.h>
#include <string.h>
#include <muil/muil.h>
#include <draw/font.h>
#include <fat.h>
#include <font.h>
#include <peripheral.h>
#include "main.h"

static const char *attribs = "RHSLDA";

static struct MuilPaneList panelist;
static MuilWidget *vbox;
static MuilWidget *hbox;
static MuilWidget *button;
static MuilWidget *button_up;
static MuilWidget *label;
static MuilWidget *listbox;
static MuilWidget *entry;

static char cwd[256] = "/";
static int selected = -1;

static void sprint_filesize(char *buf, uint32_t filesize) {
	if(filesize < 1024)
		sprintf(buf, "%u", filesize);
	else if(filesize < 1024*1024)
		sprintf(buf, "%uk", filesize/1024U);
	else
		sprintf(buf, "%uM", filesize/(1024U*1024U));
}

static void list_dir(const char *path, MuilWidget *listbox) {
	int files, i, j, k, fd;
	char row[64];
	char attrib[8];
	char fsize[12];
	char pathbuf[256];
	
	uint8_t stat;
	struct FATDirList list[8];
	char *buf;
	
	muil_listbox_clear(listbox);
	
	for(files = 0; (i = fat_dirlist(path, list, 8, files)); files += i) {
		for (j = i - 1; j >= 0; j--) {
			if(list[j].filename[0]) {
				stat = list[j].attrib;
				
				//skip volume labels
				if(stat & 0x8)
					continue;
				
				if(list[j].filename[0] == '.')
					continue;
				
				
				buf = attrib;
				for(k = 5; k != ~0; k--) {
					if(stat & (0x1 << k))
						*buf++ = attribs[k];
					else
						*buf++ = '-';
				}
				*buf = 0;
				
				if(stat & 0x10) {
					/* Directory */
					sprintf(row, "%s/", list[j].filename);
				} else {
					sprintf(pathbuf, "%s%s", list[j].filename);
					fd = fat_open(pathbuf, O_RDONLY);
					sprint_filesize(fsize, fat_fsize(fd));
					fat_close(fd);
					
					sprintf(row, "%s", list[j].filename);
				}
				
				muil_listbox_add(listbox, row);
			}
		}
	}
}

void button_callback() {
	char pathbuf[256];
	MuilPropertyValue v;
	
	v.p = muil_listbox_get(listbox, selected);
	sprintf(pathbuf, "%s%s", cwd, v.p);
	
	play(pathbuf);
	muil_pane_resize(panelist.pane, panelist.pane->x, panelist.pane->y, panelist.pane->w, panelist.pane->h);
}

void button_up_callback() {
	MuilPropertyValue v;
	
	char *last;
	if(strlen(cwd) == 1)
		return;
	
	last = strrchr(cwd, '/');
	*last = 0;
	
	last = strrchr(cwd, '/');
	last[1] = 0;
	
	button->enabled = false;
	
	list_dir(cwd, listbox);
	v.p = cwd;
	entry->set_prop(entry, MUIL_ENTRY_PROP_TEXT, v);
}

void listbox_callback() {
	MuilPropertyValue v;
	char buf[256];
	int sel;
	int r;
	
	v = listbox->get_prop(listbox, MUIL_LISTBOX_PROP_SELECTED);
	sel = v.i;
	if(sel < 0) {
		selected = sel;
		return;
	}
	
	v.p = muil_listbox_get(listbox, sel);
	r = sprintf(buf, "%s%s", cwd, v.p);
	if(fat_get_stat(buf) & 0x10) {
		button->enabled = false;
		if(sel == selected) {
			sprintf(cwd, "%s", buf);
			if(r > 0)
				buf[r - 1] = 0;
			list_dir(buf, listbox);
		}
	} else {
		button->enabled = true;
	}
	
	v.p = cwd;
	entry->set_prop(entry, MUIL_ENTRY_PROP_TEXT, v);
	selected = sel;
}

void browse() {
	MuilPropertyValue v;
	panelist.pane = muil_pane_create(20, 20, 320, 480 - 40, vbox = muil_widget_create_vbox());
	panelist.next = NULL;

	hbox = muil_widget_create_hbox();
	muil_hbox_add_child(hbox, button_up = muil_widget_create_button_text(font_small, "Up"), 0);
	muil_hbox_add_child(hbox, entry = muil_widget_create_entry(font_small), 1);
	
	muil_vbox_add_child(vbox, label = muil_widget_create_label(font_small, "Load MOD file"), 0);
	muil_vbox_add_child(vbox, hbox, 0);
	muil_vbox_add_child(vbox, listbox = muil_widget_create_listbox(font_small), 1);
	muil_vbox_add_child(vbox, muil_widget_create_spacer(4), 0);
	muil_vbox_add_child(vbox, button = muil_widget_create_button_text(font_small, "Play"), 0);
	
	button->enabled = false;
	
	list_dir(cwd, listbox);
	v.p = cwd;
	entry->set_prop(entry, MUIL_ENTRY_PROP_TEXT, v);
	
	button_up->event_handler->add(button_up, button_up_callback, MUIL_EVENT_TYPE_UI_WIDGET_ACTIVATE);
	button->event_handler->add(button, button_callback, MUIL_EVENT_TYPE_UI_WIDGET_ACTIVATE);
	listbox->event_handler->add(listbox, listbox_callback, MUIL_EVENT_TYPE_UI_WIDGET_ACTIVATE);
	muil_events(&panelist, true);
	
	for(;;) {
		volatile uint32_t *interrupt_hw = (volatile uint32_t *) PERIPHERAL_INTERRUPT_BASE;
		
		muil_events(&panelist, true);
		
		while(!interrupt_hw[32 + 10]);
		interrupt_hw[32 + 10] = 0x0;
	}
}
