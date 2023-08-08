#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <endian.h>

#include "eeptypes.h"

struct header_t header;
struct atom_t atom;
struct vendor_info_d vinf;
unsigned char* data;

int read_bin(char *in, char *outf) {

	uint16_t crc;
	FILE *fp, *out;
	int i, size_in;
	uint32_t j;
	
	fp=fopen(in, "r");
	if (!fp) {
		printf("Error reading file %s\n", in);
		return -1;
	}

	i = fseek(fp,0,SEEK_END);
	if (i) {
		printf("error %i\n", i);
		fclose(fp);
		return -1;
	}
	size_in = ftell(fp);
	rewind(fp);

	if (size_in < 0) {
		printf("Error reading size of file %s\n", in);
		fclose(fp);
		return -1;
	}

	out=fopen(outf, "w");
	if (!out) {
		printf("Error writing file %s\n", outf);
		return -1;
	}
	
	if (!fread(&header, sizeof(header), 1, fp)) {
		printf("error reading header\n");
		goto err;
	}
	
	fprintf(out, "# ---------- Dump generated by eepdump handling format version 0x%02x ----------\n#\n", FORMAT_VERSION);
	
	if (FORMAT_VERSION!=header.ver) fprintf(out, "# WARNING: format version mismatch!!!\n");
	if (HEADER_SIGN!=header.signature) fprintf(out, "# WARNING: format signature mismatch!!!\n");
	if ((uint32_t)size_in < header.eeplen) fprintf(out, "# WARNING: Incomplete file\n");
	
	fprintf(out, "# --Header--\n# signature=0x%08x\n# version=0x%02x\n# reserved=%u\n# numatoms=%u\n# eeplen=%u\n# ----------\n\n\n", header.signature, header.ver, header.res, header.numatoms, header.eeplen);
				
	if (FORMAT_VERSION!=header.ver && HEADER_SIGN!=header.signature) {
		printf("header version and signature mismatch, maybe wrong file?\n");
		goto err;
	}

	size_in -= sizeof(header);

	for (i = 0; i<header.numatoms; i++) {
		
		if (!fread(&atom, ATOM_SIZE-CRC_SIZE, 1, fp)) {
			printf("error reading atom[%i]\n", i);
			goto err;
		}
		
		printf("Reading atom %d (type = 0x%04x (", i, atom.type);
		switch(atom.type) {
			case ATOM_VENDOR_TYPE:
				printf("vendor type");
				break;
			case ATOM_DT_TYPE:
				printf("Linux device tree blob");
				break;
			case ATOM_CUSTOM_TYPE:
				printf("manufacturer custom data");
				break;
			default:
				printf("unknown");
				break;
		}
		printf("), length = %i bytes)...\n", atom.dlen);
		
		fprintf(out, "# Start of atom #%u of type 0x%04x and length %u\n", atom.count, atom.type, atom.dlen);
		
		if (atom.count != i) {
			printf("Error: atom count mismatch\n");
			fprintf(out, "# Error: atom count mismatch\n");
		}

		if ((uint32_t)size_in < atom.dlen) {
			printf("size of atom[%i] = %i longer than rest of file (%i)\n", i, atom.dlen, size_in);
			goto err;
		}
		size_in -= atom.dlen + 8; // 8 for the type, count and length (that isn't include in the length)

		long pos = ftell(fp);
		char *atom_data = (char *) malloc(atom.dlen + ATOM_SIZE-CRC_SIZE);
		memcpy(atom_data, &atom, ATOM_SIZE-CRC_SIZE);
		if (!fread(atom_data+ATOM_SIZE-CRC_SIZE, atom.dlen, 1, fp)) {
			printf("error reading atom data[%i]\n", i);
			goto err;
		}
		uint16_t calc_crc = getcrc(atom_data, atom.dlen-CRC_SIZE+ATOM_SIZE-CRC_SIZE);
		fseek(fp, pos, SEEK_SET);
		
		if (atom.type==ATOM_VENDOR_TYPE) {
			//decode vendor info
			
			if (!fread(&vinf, VENDOR_SIZE, 1, fp)) {
				printf("error reading vendor info\n");
				goto err;
			}
			
			fprintf(out, "# Vendor info\n");
			fprintf(out, "product_uuid %08x-%04x-%04x-%04x-%04x%08x\n",
				vinf.serial[3],
				vinf.serial[2]>>16, vinf.serial[2] & 0xffff,
				vinf.serial[1]>>16, vinf.serial[1] & 0xffff,
				vinf.serial[0]);
			fprintf(out, "product_id 0x%04x\n", vinf.pid);
			fprintf(out, "product_ver 0x%04x\n", vinf.pver);
			
			vinf.vstr = (char *) malloc(vinf.vslen+1);
			vinf.pstr = (char *) malloc(vinf.pslen+1);
			
			if (!fread(vinf.vstr, vinf.vslen, 1, fp)) goto err;
			if (!fread(vinf.pstr, vinf.pslen, 1, fp)) goto err;
			//close strings
			vinf.vstr[vinf.vslen] = 0;
			vinf.pstr[vinf.pslen] = 0;
			
			fprintf(out, "vendor \"%s\"   # length=%u\n", vinf.vstr, vinf.vslen);
			fprintf(out, "product \"%s\"   # length=%u\n", vinf.pstr, vinf.pslen);
			
			if (!fread(&crc, CRC_SIZE, 1, fp)) goto err;

		} else if (atom.type==ATOM_DT_TYPE) {
			//decode DT blob
			
			fprintf(out, "dt_blob");
			data = (unsigned char *) malloc(atom.dlen-CRC_SIZE);
			if (!fread(data, atom.dlen-CRC_SIZE, 1, fp)) goto err;
			
			for (j = 0; j<atom.dlen-CRC_SIZE; j++) {
				if (j % 16 == 0) fprintf(out, "\n");
				fprintf(out, "%02X ", *(data+j));
			}
			
			fprintf(out, "\n");
			
			if (!fread(&crc, CRC_SIZE, 1, fp)) goto err;
			
		} else if (atom.type==ATOM_CUSTOM_TYPE) {
			//decode custom data
			
			fprintf(out, "custom_data");
			data = (unsigned char *) malloc(atom.dlen-CRC_SIZE);
			if (!fread(data, atom.dlen-CRC_SIZE, 1, fp)) goto err;
			
			for (j = 0; j<atom.dlen-CRC_SIZE; j++) {
				if (j % 16 == 0) fprintf(out, "\n");
				fprintf(out, "%02X ", *(data+j));
			}
			
			fprintf(out, "\n");
			
			if (!fread(&crc, CRC_SIZE, 1, fp)) goto err;
			
			
		} else {
			printf("Error: unrecognised atom type\n");
			fprintf(out, "# Error: unrecognised atom type\n");
			goto err;
		}
		
		fprintf(out, "# End of atom. CRC16=0x%04x\n", crc);
		
		if (calc_crc != crc) {
			printf("Error: atom CRC16 mismatch\n");
			fprintf(out, "# Error: atom CRC16 mismatch. Calculated CRC16=0x%02x", crc);
		} else printf("CRC OK\n");
		
		fprintf(out, "\n\n");
	
	}
	
	//Total length checks. We need header.eeplen=current_position=file_length.
	long pos = ftell(fp);
	fseek(fp, 0L, SEEK_END);
	
	if (pos!=ftell(fp)) printf("Warning: Dump finished before EOF\n");
	if (pos!=(long)header.eeplen) printf("Warning: Dump finished before length specified in header\n");
	if (ftell(fp)!=(long)header.eeplen) printf("Warning: EOF does not match length specified in header\n");
	if (size_in) printf("Warning, %i bytes of file not processed\n", size_in);

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