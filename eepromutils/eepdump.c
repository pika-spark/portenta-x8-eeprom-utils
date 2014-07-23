#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "eeptypes.h"

struct header_t header;
struct atom_t atom;
struct vendor_info_d vinf;
struct gpio_map_d gpiomap;
unsigned char* data;

int read_bin(char *in, char *outf) {

	unsigned short crc;
	FILE *fp, *out;
	int i,j;
	
	fp=fopen(in, "r");
	if (!fp) {
		printf("Error reading file %s\n", in);
		return -1;
	}
	
	out=fopen(outf, "w");
	if (!out) {
		printf("Error writing file %s\n", outf);
		return -1;
	}
	
	if (!fread(&header, sizeof(header), 1, fp)) goto err;
	
	fprintf(out, "# ---------- Dump generated by eepdump handling format version 0x%04x ----------\n#\n", FORMAT_VERSION);
	
	if (FORMAT_VERSION!=header.ver) fprintf(out, "# WARNING: format version mismatch!!!\n");
	
	fprintf(out, "# --Header--\n# signature=0x%08x\n# version=%u\n# reserved=%u\n# numatoms=%u\n# eeplen=%u\n# ----------\n\n\n", header.signature, header.ver, header.res, header.numatoms, header.eeplen);
				
	
	for (i = 0; i<header.numatoms; i++) {
	
		if (!fread(&atom, ATOM_SIZE-2, 1, fp)) goto err;
		
		printf("Reading atom %d...\n", i);
		
		fprintf(out, "# Atom %u of type 0x%04x and length %u\n", atom.count, atom.type, atom.dlen);
		
		unsigned long pos = ftell(fp);
		char *atom_data = (char *) malloc(atom.dlen + ATOM_SIZE-2);
		memcpy(atom_data, &atom, ATOM_SIZE-2);
		if (!fread(atom_data+ATOM_SIZE-2, atom.dlen, 1, fp)) goto err;
		unsigned short calc_crc = getcrc(atom_data, atom.dlen-2+ATOM_SIZE-2);
		fseek(fp, pos, SEEK_SET);
		
		if (atom.type==1) {
			//decode vendor info
			
			if (!fread(&vinf, 10, 1, fp)) goto err;
			
			fprintf(out, "# Vendor info\n");
			fprintf(out, "product_serial 0x%08x\n", vinf.serial);
			fprintf(out, "product_id 0x%04x\n", vinf.pid);
			fprintf(out, "product_ver 0x%04x\n", vinf.pver);
			
			vinf.vstr = (char *) malloc(vinf.vslen);
			vinf.pstr = (char *) malloc(vinf.pslen);
			
			if (!fread(vinf.vstr, vinf.vslen, 1, fp)) goto err;
			if (!fread(vinf.pstr, vinf.pslen, 1, fp)) goto err;
			
			fprintf(out, "vendor \"%s\"\n", vinf.vstr);
			fprintf(out, "product \"%s\"\n", vinf.pstr);
			
			fprintf(out, "\n\n");
			
			if (!fread(&crc, 2, 1, fp)) goto err;
			
		} else if (atom.type==2) {
			//decode GPIO map
			if (!fread(&gpiomap, 30, 1, fp)) goto err;
			
			fprintf(out, "# GPIO map info\n");
			fprintf(out, "gpio_drive %d\n", gpiomap.flags & 15); //1111
			fprintf(out, "gpio_slew %d\n", (gpiomap.flags & 48)>>4); //110000
			fprintf(out, "gpio_hyteresis %d\n", (gpiomap.flags & 192)>>6); //11000000
			
			fprintf(out, "#        GPIO  FUNCTION  PULL\n#        ----  --------  ----\n");

			for (j = 0; j<28; j++) {
				if (gpiomap.pins[j] & (1<<7)) {
					//board uses this pin
					
					char *pull_str = "INVALID";
					switch ((gpiomap.pins[j] & 96)>>5) { //1100000
						case 0:	pull_str = "DEFAULT";
								break;
						case 1: pull_str = "UP";
								break;
						case 2: pull_str = "DOWN";
								break;
						case 3: pull_str = "NONE";
								break;
					}
					
					char *func_str = "INVALID";
					switch ((gpiomap.pins[j] & 7)) { //111
						case 0:	func_str = "INPUT";
								break;
						case 1: func_str = "OUTPUT";
								break;
						case 4: func_str = "ALT0";
								break;
						case 5: func_str = "ALT1";
								break;
						case 6: func_str = "ALT2";
								break;
						case 7: func_str = "ALT3";
								break;
						case 3: func_str = "ALT4";
								break;
						case 2: func_str = "ALT5";
								break;
					}
					
					fprintf(out, "setgpio  %d      %s     %s\n", j, func_str, pull_str);
				}
			}
			
			fprintf(out, "\n\n");
			
			if (!fread(&crc, 2, 1, fp)) goto err;
			
		} else if (atom.type==3) {
			//decode DT blob
			
			fprintf(out, "dt_blob");
			data = (char *) malloc(atom.dlen-2);
			if (!fread(data, atom.dlen-2, 1, fp)) goto err;
			
			for (j = 0; j<atom.dlen-2; j++) {
				if (j % 16 == 0) fprintf(out, "\n");
				fprintf(out, "%02X ", *(data+j));
			}
			
			fprintf(out, "\n\n\n");
			
			if (!fread(&crc, 2, 1, fp)) goto err;
			
		} else if (atom.type==4) {
			//decode custom data
			
			fprintf(out, "custom_data");
			data = (char *) malloc(atom.dlen-2);
			if (!fread(data, atom.dlen-2, 1, fp)) goto err;
			
			for (j = 0; j<atom.dlen-2; j++) {
				if (j % 16 == 0) fprintf(out, "\n");
				fprintf(out, "%02X ", *(data+j));
			}
			
			fprintf(out, "\n\n\n");
			
			if (!fread(&crc, 2, 1, fp)) goto err;
			
			
		} else {
			printf("Error: unrecognised atom type\n");
			fprintf(out, "# Error: unrecognised atom type\n");
			goto err;
		}
		
		
		if (calc_crc != crc) {
			printf("Error: atom CRC16 mismatch\n");
			fprintf(out, "# Error: atom CRC16 mismatch. Claimed CRC16=0x%02x, calculated CRC16=0x%02x", crc, calc_crc);
		} else printf("CRC OK\n");
		
	
	}
	
	printf("Done.\n");
	
	fclose(fp);
	fclose(out);
	return 0;
	
err:
	printf("Unexpected EOF or error occurred\n");
	fclose(fp);
	fclose(out);
	return 0;
}


int main(int argc, char *argv[]) {
	int ret;
	int i;
	
	if (argc<3) {
		printf("Wrong input format.\n");
		printf("Try 'eepdump input_file output_file'\n");
		return 0;
	}
	
	
	ret = read_bin(argv[1], argv[2]);
	if (ret) {
		printf("Error reading input, aborting\n");
		return 0;
	}
	

	return 0;
}