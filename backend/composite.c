/* composite.c - Handles UCC.EAN Composite Symbols */

/*
    libzint - the open source barcode library
    Copyright (C) 2008 Robin Stuart <robin@zint.org.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/* The functions "getBit", "init928" and "encode928" are copyright BSI and are
   released with permission under the following terms:
   
   "Copyright subsists in all BSI publications. BSI also holds the copyright, in the
   UK, of the international standardisation bodies. Except as
   permitted under the Copyright, Designs and Patents Act 1988 no extract may be
   reproduced, stored in a retrieval system or transmitted in any form or by any
   means - electronic, photocopying, recording or otherwise - without prior written
   permission from BSI.
   
   "This does not preclude the free use, in the course of implementing the standard,
   of necessary details such as symbols, and size, type or grade designations. If these
   details are to be used for any other purpose than implementation then the prior
   written permission of BSI must be obtained."
   
   The date of publication for these functions is 31 May 2006
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "common.h"
#include "large.h"
#include "composite.h"
#include "pdf417.h"

#define UINT unsigned short

int general_rules(char field[], char type[]);
int eanx(struct zint_symbol *symbol, unsigned char source[]);
int ean_128(struct zint_symbol *symbol, unsigned char source[]);
int rss14(struct zint_symbol *symbol, unsigned char source[]);
int rsslimited(struct zint_symbol *symbol, unsigned char source[]);
int rssexpanded(struct zint_symbol *symbol, unsigned char source[]);

static UINT pwr928[69][7];

int min(int first, int second) {
	if(first <= second) {
		return first;
	} else {
		return second;
	}
}

/* gets bit in bitString at bitPos */
int getBit(UINT *bitStr, int bitPos) {
	return(((bitStr[bitPos/16] & (0x8000>>(bitPos%16))) == 0) ? 0 : 1);
}

/* initialize pwr928 encoding table */
void init928(void) {
	int i, j, v;
	int cw[7];
	cw[6] = 1L;
	for (i = 5; i >= 0; i--)
		cw[i] = 0;
	
	for (i = 0; i < 7; i++)
		pwr928[0][i] = cw[i];
	for (j = 1; j < 69; j++) {
		for (v = 0, i = 6; i >= 1; i--) {
			v = (2 * cw[i]) + (v / 928);
			pwr928[j][i] = cw[i] = v % 928;
		}
		pwr928[j][0] = cw[0] = (2 * cw[0]) + (v / 928);
	}
	return;
}

/* converts bit string to base 928 values, codeWords[0] is highest order */
int encode928(UINT bitString[], UINT codeWords[], int bitLng) {
	int i, j, b, bitCnt, cwNdx, cwCnt, cwLng;
	for (cwNdx = cwLng = b = 0; b < bitLng; b += 69, cwNdx += 7) {
		bitCnt = min(bitLng-b, 69);
		cwLng += cwCnt = bitCnt/10 + 1;
		for (i = 0; i < cwCnt; i++)
			codeWords[cwNdx+i] = 0; /* init 0 */
		for (i = 0; i < bitCnt; i++) {
			if (getBit(bitString, b+bitCnt-i-1)) {
				for (j = 0; j < cwCnt; j++)
					codeWords[cwNdx+j] += pwr928[i][j+7-cwCnt];
			}
		}
		for (i = cwCnt-1; i > 0; i--) {
			/* add "carries" */
			codeWords[cwNdx+i-1] += codeWords[cwNdx+i]/928L;
			codeWords[cwNdx+i] %= 928L;
		}
	}
	return (cwLng);
}

int cc_a(struct zint_symbol *symbol, unsigned char source[], int cc_width)
{ /* CC-A 2D component */
	int i, strpos, segment, bitlen, cwCnt, variant, rows;
	int k, offset, j, total, rsCodeWords[8];
	int LeftRAPStart, RightRAPStart, CentreRAPStart, StartCluster;
	int LeftRAP, RightRAP, CentreRAP, Cluster, dummy[5];
	int writer, flip, loop;
	UINT codeWords[28];
	UINT bitStr[13];
	char codebarre[100], pattern[580];

	variant=0;

	for(i = 0; i < 13; i++) { bitStr[i] = 0; }
	for(i = 0; i < 28; i++) { codeWords[i] = 0; }
	
	bitlen = ustrlen(source);
	
	for(i = bitlen; i < 208; i++) {
		source[i] = '0';
	}
	source[208] = '\0';
	
	for(segment = 0; segment < 13; segment++) {
		strpos = segment * 16;
		if(source[strpos] == '1') { bitStr[segment] += 0x8000; }
		if(source[strpos + 1] == '1') { bitStr[segment] += 0x4000; }
		if(source[strpos + 2] == '1') { bitStr[segment] += 0x2000; }
		if(source[strpos + 3] == '1') { bitStr[segment] += 0x1000; }
		if(source[strpos + 4] == '1') { bitStr[segment] += 0x800; }
		if(source[strpos + 5] == '1') { bitStr[segment] += 0x400; }
		if(source[strpos + 6] == '1') { bitStr[segment] += 0x200; }
		if(source[strpos + 7] == '1') { bitStr[segment] += 0x100; }
		if(source[strpos + 8] == '1') { bitStr[segment] += 0x80; }
		if(source[strpos + 9] == '1') { bitStr[segment] += 0x40; }
		if(source[strpos + 10] == '1') { bitStr[segment] += 0x20; }
		if(source[strpos + 11] == '1') { bitStr[segment] += 0x10; }
		if(source[strpos + 12] == '1') { bitStr[segment] += 0x08; }
		if(source[strpos + 13] == '1') { bitStr[segment] += 0x04; }
		if(source[strpos + 14] == '1') { bitStr[segment] += 0x02; }
		if(source[strpos + 15] == '1') { bitStr[segment] += 0x01; }
	}
	
	init928();
	/* encode codeWords from bitStr */
	cwCnt = encode928(bitStr, codeWords, bitlen);
	
	switch(cc_width) {
		case 2:
			switch(cwCnt) {
				case 6: variant = 0; break;
				case 8: variant = 1; break;
				case 9: variant = 2; break;
				case 11: variant = 3; break;
				case 12: variant = 4; break;
				case 14: variant = 5; break;
				case 17: variant = 6; break;
			}
			break;
		case 3:
			switch(cwCnt) {
				case 8: variant = 7; break;
				case 10: variant = 8; break;
				case 12: variant = 9; break;
				case 14: variant = 10; break;
				case 17: variant = 11; break;
			}
			break;
		case 4:
			switch(cwCnt) {
				case 8: variant = 12; break;
				case 11: variant = 13; break;
				case 14: variant = 14; break;
				case 17: variant = 15; break;
				case 20: variant = 16; break;
			}
			break;
	}
	
	rows = ccaVariants[variant];
	k = ccaVariants[17 + variant];
	offset = ccaVariants[34 + variant];
	
	/* Reed-Solomon error correction */

	for(i = 0; i < 8; i++) {
		rsCodeWords[i] = 0;
	}
	total = 0;
	for(i = 0; i < cwCnt; i++) {
		total = (codeWords[i] + rsCodeWords[k - 1]) % 929;
		for(j = k - 1; j >= 0; j--) {
			if(j == 0) {
				rsCodeWords[j] = (929 - (total * ccaCoeffs[offset + j]) % 929) % 929;
			} else {
				rsCodeWords[j] = (rsCodeWords[j - 1] + 929 - (total * ccaCoeffs[offset + j]) % 929) % 929;
			}
		}
	}
	
	for(j = 0; j < k; j++) {
		if(rsCodeWords[j] != 0) { rsCodeWords[j] = 929 - rsCodeWords[j]; }
	}
	
	for(i = k - 1; i >= 0; i--) {
		codeWords[cwCnt] = rsCodeWords[i];
		cwCnt++;
	}
	
	/* Place data into table */
	LeftRAPStart = aRAPTable[variant];
	CentreRAPStart = aRAPTable[variant + 17];
	RightRAPStart = aRAPTable[variant + 34];
	StartCluster = aRAPTable[variant + 51] / 3;
	
	LeftRAP = LeftRAPStart;
	CentreRAP = CentreRAPStart;
	RightRAP = RightRAPStart;
	Cluster = StartCluster; /* Cluster can be 0, 1 or 2 for Cluster(0), Cluster(3) and Cluster(6) */
	
	for(i = 0; i < rows; i++) {
		strcpy(codebarre, "");
		offset = 929 * Cluster;
		for(j = 0; j < 5; j++) {
			dummy[j] = 0;
		}
		for(j = 0; j < cc_width ; j++) {
			dummy[j + 1] = codeWords[i * cc_width + j];
		}
		/* Copy the data into codebarre */
		concat(codebarre, RAPLR[LeftRAP]);
		concat(codebarre, "1");
		concat(codebarre, codagemc[offset + dummy[1]]);
		concat(codebarre, "1");
		if(dummy[2] != 0) {
			concat(codebarre, "1");
			concat(codebarre, codagemc[offset + dummy[2]]);
			concat(codebarre, "1");
		}
		if(CentreRAPStart != 0) {
			concat(codebarre, RAPC[CentreRAP]);
		}
		if(dummy[3] != 0) {
			concat(codebarre, "1");
			concat(codebarre, codagemc[offset + dummy[3]]);
			concat(codebarre, "1");
		}
		if(dummy[4] != 0) {
			concat(codebarre, "1");
			concat(codebarre, codagemc[offset + dummy[4]]);
			concat(codebarre, "1");
		}
		concat(codebarre, RAPLR[RightRAP]);
		concat(codebarre, "1"); /* stop */
		
		/* Now codebarre is a mixture of letters and numbers */
		
		writer = 0;
		flip = 1;
		strcpy(pattern, "");
		for(loop = 0; loop < strlen(codebarre); loop++) {
			if((codebarre[loop] >= '0') && (codebarre[loop] <= '9')) {
				for(k = 0; k < ctoi(codebarre[loop]); k++) {
					if(flip == 0) {
						pattern[writer] = '0';
					} else {
						pattern[writer] = '1';
					}
					writer++;
				}
				pattern[writer] = '\0';
				if(flip == 0) {
					flip = 1;
				} else {
					flip = 0;
				}
			} else {
				lookup(BRSET, PDFttf, codebarre[loop], pattern);
				writer += 5;
			}
		}
		symbol->width = writer;
		
		/* so now pattern[] holds the string of '1's and '0's. - copy this to the symbol */
		for(loop = 0; loop < strlen(pattern); loop++) {
			symbol->encoded_data[i][loop] = pattern[loop];
		}
		symbol->row_height[i] = 2;
		symbol->rows++;
		
		/* Set up RAPs and Cluster for next row */
		LeftRAP++;
		CentreRAP++;
		RightRAP++;
		Cluster++;
		
		if(LeftRAP == 53) {
			LeftRAP = 1;
		}
		if(CentreRAP == 53) {
			CentreRAP = 1;
		}
		if(RightRAP == 53) {
			RightRAP = 1;
		}
		if(Cluster == 3) {
			Cluster = 0;
		}
	}
	
	return 0;
}

int cc_b(struct zint_symbol *symbol, unsigned char source[], int cc_width)
{ /* CC-B 2D component */
	int length, i, binloc;
	unsigned char data_string[(ustrlen(source) / 8) + 3];
	int chainemc[180], mclength;
	int k, j, longueur, mccorrection[50], offset;
	int total, dummy[5];
	char codebarre[100], pattern[580];
	int variant, LeftRAPStart, CentreRAPStart, RightRAPStart, StartCluster;
	int LeftRAP, CentreRAP, RightRAP, Cluster, writer, flip, loop;
	
	length = ustrlen(source) / 8;
	
	for(i = 0; i < length; i++) {
		binloc = i * 8;
		
		data_string[i] = 0;
		if(source[binloc] == '1') { data_string[i] += 0x80; }
		if(source[binloc + 1] == '1') { data_string[i] += 0x40; }
		if(source[binloc + 2] == '1') { data_string[i] += 0x20; }
		if(source[binloc + 3] == '1') { data_string[i] += 0x10; }
		if(source[binloc + 4] == '1') { data_string[i] += 0x08; }
		if(source[binloc + 5] == '1') { data_string[i] += 0x04; }
		if(source[binloc + 6] == '1') { data_string[i] += 0x02; }
		if(source[binloc + 7] == '1') { data_string[i] += 0x01; }
	}
	
	
	mclength = 0;
	
	/* "the CC-B component shall have codeword 920 in the first symbol character position" (section 9a) */
	chainemc[mclength] = 920;
	mclength++;
	
	byteprocess(chainemc, &mclength, data_string, 0, length, 0, 0x00);
	
	/* Now figure out which variant of the symbol to use and load values accordingly */
	
	variant = 0;
	
	if(cc_width == 2) {
		variant = 13;
		if(mclength <= 33) { variant = 12; }
		if(mclength <= 29) { variant = 11; }
		if(mclength <= 24) { variant = 10; }
		if(mclength <= 19) { variant = 9; }
		if(mclength <= 13) { variant = 8; }
		if(mclength <= 8) { variant = 7; }
	}

	if(cc_width == 3) {
		variant = 23;
		if(mclength <= 70) { variant = 22; }
		if(mclength <= 58) { variant = 21; }
		if(mclength <= 46) { variant = 20; }
		if(mclength <= 34) { variant = 19; }
		if(mclength <= 24) { variant = 18; }
		if(mclength <= 18) { variant = 17; }
		if(mclength <= 14) { variant = 16; }
		if(mclength <= 10) { variant = 15; }
		if(mclength <= 6) { variant = 14; }
	}

	if(cc_width == 4) {
		variant = 34;
		if(mclength <= 108) { variant = 33; }
		if(mclength <= 90) { variant = 32; }
		if(mclength <= 72) { variant = 31; }
		if(mclength <= 54) { variant = 30; }
		if(mclength <= 39) { variant = 29; }
		if(mclength <= 30) { variant = 28; }
		if(mclength <= 24) { variant = 27; }
		if(mclength <= 18) { variant = 26; }
		if(mclength <= 12) { variant = 25; }
		if(mclength <= 8) { variant = 24; }
	}

	/* Now we have the variant we can load the data - from here on the same as MicroPDF417 code */
	variant --;
	symbol->option_2 = MicroVariants[variant]; /* columns */
	symbol->rows = MicroVariants[variant + 34]; /* rows */
	k = MicroVariants[variant + 68]; /* number of EC CWs */
	longueur = (symbol->option_2 * symbol->rows) - k; /* number of non-EC CWs */
	i = longueur - mclength; /* amount of padding required */
	offset = MicroVariants[variant + 102]; /* coefficient offset */
	
	/* We add the padding */
	while (i > 0) {
		chainemc[mclength] = 900;
		mclength++;
		i--;
	}
	
	/* Reed-Solomon error correction */
	longueur = mclength;
	for(loop = 0; loop < 50; loop++) {
		mccorrection[loop] = 0;
	}
	total = 0;
	for(i = 0; i < longueur; i++) {
		total = (chainemc[i] + mccorrection[k - 1]) % 929;
		for(j = k - 1; j >= 0; j--) {
			if(j == 0) {
				mccorrection[j] = (929 - (total * Microcoeffs[offset + j]) % 929) % 929;
			} else {
				mccorrection[j] = (mccorrection[j - 1] + 929 - (total * Microcoeffs[offset + j]) % 929) % 929;
			}
		}
	}
	
	for(j = 0; j < k; j++) {
		if(mccorrection[j] != 0) { mccorrection[j] = 929 - mccorrection[j]; }
	}
	/* we add these codes to the string */
	for(i = k - 1; i >= 0; i--) {
		chainemc[mclength] = mccorrection[i];
		mclength++;
	}
	
	/* Now get the RAP (Row Address Pattern) start values */
	LeftRAPStart = RAPTable[variant];
	CentreRAPStart = RAPTable[variant + 34];
	RightRAPStart = RAPTable[variant + 68];
	StartCluster = RAPTable[variant + 102] / 3;
	
	/* That's all values loaded, get on with the encoding */
	
	LeftRAP = LeftRAPStart;
	CentreRAP = CentreRAPStart;
	RightRAP = RightRAPStart;
	Cluster = StartCluster; /* Cluster can be 0, 1 or 2 for Cluster(0), Cluster(3) and Cluster(6) */
	
	for(i = 0; i < symbol->rows; i++) {
		strcpy(codebarre, "");
		offset = 929 * Cluster;
		for(j = 0; j < 5; j++) {
			dummy[j] = 0;
		}
		for(j = 0; j < symbol->option_2 ; j++) {
			dummy[j + 1] = chainemc[i * symbol->option_2 + j];
		}
		/* Copy the data into codebarre */
		concat(codebarre, RAPLR[LeftRAP]);
		concat(codebarre, "1");
		concat(codebarre, codagemc[offset + dummy[1]]);
		concat(codebarre, "1");
		if(cc_width == 3) {
			concat(codebarre, RAPC[CentreRAP]);
		}
		if(dummy[2] != 0) {
			concat(codebarre, "1");
			concat(codebarre, codagemc[offset + dummy[2]]);
			concat(codebarre, "1");
		}
		if(cc_width == 4) {
			concat(codebarre, RAPC[CentreRAP]);
		}
		if(dummy[3] != 0) {
			concat(codebarre, "1");
			concat(codebarre, codagemc[offset + dummy[3]]);
			concat(codebarre, "1");
		}
		if(dummy[4] != 0) {
			concat(codebarre, "1");
			concat(codebarre, codagemc[offset + dummy[4]]);
			concat(codebarre, "1");
		}
		concat(codebarre, RAPLR[RightRAP]);
		concat(codebarre, "1"); /* stop */
		
		/* Now codebarre is a mixture of letters and numbers */
		
		writer = 0;
		flip = 1;
		strcpy(pattern, "");
		for(loop = 0; loop < strlen(codebarre); loop++) {
			if((codebarre[loop] >= '0') && (codebarre[loop] <= '9')) {
				for(k = 0; k < ctoi(codebarre[loop]); k++) {
					if(flip == 0) {
						pattern[writer] = '0';
					} else {
						pattern[writer] = '1';
					}
					writer++;
				}
				pattern[writer] = '\0';
				if(flip == 0) {
					flip = 1;
				} else {
					flip = 0;
				}
			} else {
				lookup(BRSET, PDFttf, codebarre[loop], pattern);
				writer += 5;
			}
		}
		symbol->width = writer;
		
		/* so now pattern[] holds the string of '1's and '0's. - copy this to the symbol */
		for(loop = 0; loop < strlen(pattern); loop++) {
			symbol->encoded_data[i][loop] = pattern[loop];
		}
		symbol->row_height[i] = 2;
		
		/* Set up RAPs and Cluster for next row */
		LeftRAP++;
		CentreRAP++;
		RightRAP++;
		Cluster++;
		
		if(LeftRAP == 53) {
			LeftRAP = 1;
		}
		if(CentreRAP == 53) {
			CentreRAP = 1;
		}
		if(RightRAP == 53) {
			RightRAP = 1;
		}
		if(Cluster == 3) {
			Cluster = 0;
		}
	}
	
	return 0;
}

int cc_c(struct zint_symbol *symbol, unsigned char source[], int cc_width, int ecc_level)
{ /* CC-C 2D component - byte compressed PDF417 */
	int length, i, binloc;
	unsigned char data_string[(ustrlen(source) / 8) + 4];
	int chainemc[1000], mclength, k;
	int offset, longueur, loop, total, j, mccorrection[520];
	int c1, c2, c3, dummy[35];
	char codebarre[100], pattern[580];
	
	length = ustrlen(source) / 8;
	
	for(i = 0; i < length; i++) {
		binloc = i * 8;
		
		data_string[i] = 0;
		if(source[binloc] == '1') { data_string[i] += 0x80; }
		if(source[binloc + 1] == '1') { data_string[i] += 0x40; }
		if(source[binloc + 2] == '1') { data_string[i] += 0x20; }
		if(source[binloc + 3] == '1') { data_string[i] += 0x10; }
		if(source[binloc + 4] == '1') { data_string[i] += 0x08; }
		if(source[binloc + 5] == '1') { data_string[i] += 0x04; }
		if(source[binloc + 6] == '1') { data_string[i] += 0x02; }
		if(source[binloc + 7] == '1') { data_string[i] += 0x01; }
	}
	
	mclength = 0;
	
	chainemc[mclength] = 0; /* space for length descriptor */
	mclength++;
	chainemc[mclength] = 920; /* CC-C identifier */
	mclength++;
	
	byteprocess(chainemc, &mclength, data_string, 0, length, 0, 0x00);
	
	chainemc[0] = mclength;
	
	k = 1;
	for(i = 1; i <= (ecc_level + 1); i++)
	{
		k *= 2;
	}
	
	/* 796 - we now take care of the Reed Solomon codes */
	switch(ecc_level) {
		case 1: offset = 2; break;
		case 2: offset = 6; break;
		case 3: offset = 14; break;
		case 4: offset = 30; break;
		case 5: offset = 62; break;
		case 6: offset = 126; break;
		case 7: offset = 254; break;
		case 8: offset = 510; break;
		default: offset = 0; break;
	}

	longueur = mclength;
	for(loop = 0; loop < 520; loop++) {
		mccorrection[loop] = 0;
	}
	total = 0;
	for(i = 0; i < longueur; i++) {
		total = (chainemc[i] + mccorrection[k - 1]) % 929;
		for(j = k - 1; j >= 0; j--) {
			if(j == 0) {
				mccorrection[j] = (929 - (total * coefrs[offset + j]) % 929) % 929;
			} else {
				mccorrection[j] = (mccorrection[j - 1] + 929 - (total * coefrs[offset + j]) % 929) % 929;
			}
		}
	}
	
	for(j = 0; j < k; j++) {
		if(mccorrection[j] != 0) { mccorrection[j] = 929 - mccorrection[j]; }
	}
	/* we add these codes to the string */
	for(i = k - 1; i >= 0; i--) {
		chainemc[mclength] = mccorrection[i];
		mclength++;
	}
	
	/* 818 - The CW string is finished */
	c1 = (mclength / cc_width - 1) / 3;
	c2 = ecc_level * 3 + (mclength / cc_width - 1) % 3;
	c3 = cc_width - 1;
	
	/* we now encode each row */
	for(i = 0; i <= (mclength / cc_width) - 1; i++) {
		for(j = 0; j < cc_width ; j++) {
			dummy[j + 1] = chainemc[i * cc_width + j];
		}
		k = (i / 3) * 30;
		switch(i % 3) {
				/* follows this pattern from US Patent 5,243,655: 
			Row 0: L0 (row #, # of rows)         R0 (row #, # of columns)
			Row 1: L1 (row #, security level)    R1 (row #, # of rows)
			Row 2: L2 (row #, # of columns)      R2 (row #, security level)
			Row 3: L3 (row #, # of rows)         R3 (row #, # of columns)
			etc. */
			case 0:
				dummy[0] = k + c1;
				dummy[cc_width + 1] = k + c3;
				break;
			case 1:
				dummy[0] = k + c2;
				dummy[cc_width + 1] = k + c1;
				break;
			case 2:
				dummy[0] = k + c3;
				dummy[cc_width + 1] = k + c2;
				break;
		}
		strcpy(codebarre, "+*"); /* Start with a start char and a separator */

		for(j = 0; j <= cc_width + 1;  j++) {
			switch(i % 3) {
				case 1: offset = 929; /* cluster(3) */ break;
				case 2: offset = 1858; /* cluster(6) */ break;
				default: offset = 0; /* cluster(0) */ break;
			}
			concat(codebarre, codagemc[offset + dummy[j]]);
			concat(codebarre, "*");
		}
		concat(codebarre, "-");
		
		strcpy(pattern, "");
		for(loop = 0; loop < strlen(codebarre); loop++) {
			lookup(BRSET, PDFttf, codebarre[loop], pattern);
		}
		for(loop = 0; loop < strlen(pattern); loop++) {
			symbol->encoded_data[i][loop] = pattern[loop];
		}
		symbol->row_height[i] = 3;
	}
	symbol->rows = (mclength / cc_width);
	symbol->width = strlen(pattern);
	
	return 0;	
}

int cc_binary_string(struct zint_symbol *symbol, unsigned char source[], char binary_string[], int cc_mode, int *cc_width, int *ecc, int lin_width)
{ /* Handles all data encodation from section 5 of ISO/IEC 24723 */
	int encoding_method, read_posn, d1, d2, value, alpha_pad;
	int group_val, i, j, mask, ai_crop, ai_crop_posn, fnc1_latch;
	int ai90_mode, latch, remainder, binary_length;
	char date_str[4];
	char general_field[ustrlen(source)], general_field_type[ustrlen(source)];
	int target_bitsize;
	
	encoding_method = 1;
	read_posn = 0;
	ai_crop = 0;
	ai_crop_posn = -1;
	fnc1_latch = 0;
	alpha_pad = 0;
	ai90_mode = 0;
	*(ecc) = 0;
	value = 0;
	target_bitsize = 0;

	if((source[0] == '1') && ((source[1] == '0') || (source[1] == '1') || (source[1] == '7')) && (ustrlen(source) > 8)) {
		/* Source starts (10), (11) or (17) */
		encoding_method = 2;
	}
	
	if((source[0] == '9') && (source[1] == '0')) {
		/* Source starts (90) */
		encoding_method = 3;
	}
	
	if(encoding_method == 1) {
		concat(binary_string, "0");
	}
	
	if(encoding_method == 2) {
		/* Encoding Method field "10" - date and lot number */
		
		concat(binary_string, "10");
		
		if(source[1] == '0') {
			/* No date data */
			concat(binary_string, "11");
			read_posn = 2;
		} else {
			/* Production Date (11) or Expiration Date (17) */
			date_str[0] = source[2];
			date_str[1] = source[3];
			date_str[2] = '\0';
			group_val = atoi(date_str) * 384;
		
			date_str[0] = source[4];
			date_str[1] = source[5];
			group_val += (atoi(date_str) - 1) * 32;
		
			date_str[0] = source[6];
			date_str[1] = source[7];
			group_val += atoi(date_str);
		
			mask = 0x8000;
			for(j = 0; j < 16; j++) {
				if((group_val & mask) == 0x00) {
					concat(binary_string, "0");
				} else {
					concat(binary_string, "1");
				}
				mask = mask >> 1;
			}
			
			if(source[1] == '1') {
				/* Production Date AI 11 */
				concat(binary_string, "0");
			} else {
				/* Expiration Date AI 17 */
				concat(binary_string, "1");
			}
			read_posn = 8;
		}
		
		if((source[read_posn] == '1') && (source[read_posn + 1] == '0')) {
			/* Followed by AI 10 - strip this from general field */
			read_posn += 2;
		} else {
			/* An FNC1 character needs to be inserted in the general field */
			fnc1_latch = 1;
		}
	}

	if (encoding_method == 3) {
		/* Encodation Method field of "11" - AI 90 */
		char ninety[ustrlen(source)], numeric_part[4];
		int alpha, alphanum, numeric, test1, test2, test3, next_ai_posn;
		int numeric_value, table3_letter, mask;
		
		/* "This encodation method may be used if an element string with an AI
		90 occurs at the start of the data message, and if the data field
		following the two-digit AI 90 starts with an alphanumeric string which
		complies with a specific format." (para 5.2.2) */
		
		i = 0;
		do {
			ninety[i] = source[i + 2];
			i++;
		} while ((source[i + 2] != '[') && ((i + 2) < ustrlen(source)));
		ninety[i] = '\0';
		
		/* Find out if the AI 90 data is alphabetic or numeric or both */
		
		alpha = 0;
		alphanum = 0;
		numeric = 0;
		
		for(i = 0; i < strlen(ninety); i++) {
			
			if ((ninety[i] >= 'A') && (ninety[i] <= 'Z')) {
				/* Character is alphabetic */
				alpha += 1;
			}
			
			if ((ninety[i] >= '0') && (ninety[i] <= '9')) {
				/* Character is numeric */
				numeric += 1;
			}
			
			switch(ninety[i]) {
				case '*':
				case ',':
				case '-':
				case '.':
				case '/': alphanum += 1; break;
			}
			
			if (!(((ninety[i] >= '0') && (ninety[i] <= '9')) || ((ninety[i] >= 'A') && (ninety[i] <= 'Z')))) {
				if((ninety[i] != '*') && (ninety[i] != ',') && (ninety[i] != '-') && (ninety[i] != '.') && (ninety[i] != '/')) {
					/* An Invalid AI 90 character */
					strcpy(symbol->errtxt, "Invalid AI 90 data [A1]");
					return ERROR_INVALID_DATA;
				}
			}
		}
		
		/* must start with 0, 1, 2 or 3 digits followed by an uppercase character */
		test1 = -1;
		for(i = 3; i >= 0; i--) {
			if ((ninety[i] >= 'A') && (ninety[i] <= 'Z')) {
				test1 = i;
			}
		}
		
		test2 = 0;
		for(i = 0; i < test1; i++) {
			if (!((ninety[i] >= '0') && (ninety[i] <= '9'))) {
				test2 = 1;
			}
		}
		
		/* leading zeros are not permitted */
		test3 = 0;
		if((test1 >= 1) && (ninety[0] == '0')) { test3 = 1; }
		
		if((test1 != -1) && (test2 != 1) && (test3 == 0)) {
			/* Encodation method "11" can be used */
			concat(binary_string, "11");
			
			numeric -= test1;
			alpha --;
			
			/* Decide on numeric, alpha or alphanumeric mode */
			/* Alpha mode is a special mode for AI 90 */
			
			if(alphanum > 0) {
				/* Alphanumeric mode */
				concat(binary_string, "0");
				ai90_mode = 1;
			} else {
				if(alpha > numeric) {
					/* Alphabetic mode */
					concat(binary_string, "11");
					ai90_mode = 2;
				} else {
					/* Numeric mode */
					concat(binary_string, "10");
					ai90_mode = 3;
				}
			}

			next_ai_posn = 2 + strlen(ninety);
			
			if(source[next_ai_posn] == '[') {
				/* There are more AIs afterwords */
				if((source[next_ai_posn + 1] == '2') && (source[next_ai_posn + 2] == '1')) {
					/* AI 21 follows */
					ai_crop = 1;
					ai_crop_posn = next_ai_posn + 1;
				}
				
				if((source[next_ai_posn + 1] == '8') && (source[next_ai_posn + 2] == '0') && (source[next_ai_posn + 3] == '0') && (source[next_ai_posn + 4] == '4')) {
					/* AI 8004 follows */
					ai_crop = 2;
					ai_crop_posn = next_ai_posn + 1;
				}
			}
			
			switch(ai_crop) {
				case 0: concat(binary_string, "0"); break;
				case 1: concat(binary_string, "10"); break;
				case 2: concat(binary_string, "11"); break;
			}

			if(test1 == 0) {
				strcpy(numeric_part, "0");
			} else {
				for(i = 0; i < test1; i++) {
					numeric_part[i] = ninety[i];
				}
				numeric_part[i] = '\0';
			}
			
			numeric_value = atoi(numeric_part);
			
			table3_letter = -1;
			if(numeric_value < 31) {
				switch(ninety[test1]) {
					case 'B': table3_letter = 0; break;
					case 'D': table3_letter = 1; break;
					case 'H': table3_letter = 2; break;
					case 'I': table3_letter = 3; break;
					case 'J': table3_letter = 4; break;
					case 'K': table3_letter = 5; break;
					case 'L': table3_letter = 6; break;
					case 'N': table3_letter = 7; break;
					case 'P': table3_letter = 8; break;
					case 'Q': table3_letter = 9; break;
					case 'R': table3_letter = 10; break;
					case 'S': table3_letter = 11; break;
					case 'T': table3_letter = 12; break;
					case 'V': table3_letter = 13; break;
					case 'W': table3_letter = 14; break;
					case 'Z': table3_letter = 15; break;
				}
			}
			
			if(table3_letter != -1) {
				/* Encoding can be done according to 5.2.2 c) 2) */
				/* five bit binary string representing value before letter */
				mask = 0x10;
				for(j = 0; j < 5; j++) {
					if((numeric_value & mask) == 0x00) {
						concat(binary_string, "0");
					} else {
						concat(binary_string, "1");
					}
					mask = mask >> 1;
				}

				/* followed by four bit representation of letter from Table 3 */
				mask = 0x08;
				for(j = 0; j < 4; j++) {
					if((table3_letter & mask) == 0x00) {
						concat(binary_string, "0");
					} else {
						concat(binary_string, "1");
					}
					mask = mask >> 1;
				}
			} else {
				/* Encoding is done according to 5.2.2 c) 3) */
				concat(binary_string, "11111");
				/* ten bit representation of number */
				mask = 0x200;
				for(j = 0; j < 10; j++) {
					if((numeric_value & mask) == 0x00) {
						concat(binary_string, "0");
					} else {
						concat(binary_string, "1");
					}
					mask = mask >> 1;
				}

				/* five bit representation of ASCII character */
				mask = 0x10;
				for(j = 0; j < 5; j++) {
					if(((ninety[test1] - 65) & mask) == 0x00) {
						concat(binary_string, "0");
					} else {
						concat(binary_string, "1");
					}
					mask = mask >> 1;
				}
			}

			read_posn = test1 + 3;
		} else {
			/* Use general field encodation instead */
			concat(binary_string, "0");
			read_posn = 0;
		}
	}
	
	/* Now encode the rest of the AI 90 data field */
	if(ai90_mode == 2) {
		/* Alpha encodation (section 5.2.3) */
		do {
			if((source[read_posn] >= '0') && (source[read_posn] <= '9')) {
				mask = 0x10;
				for(j = 0; j < 5; j++) {
					if(((source[read_posn] + 4) & mask) == 0x00) {
						concat(binary_string, "0");
					} else {
						concat(binary_string, "1");
					}
					mask = mask >> 1;
				}
			}
			
			if((source[read_posn] >= 'A') && (source[read_posn] <= 'Z')) {
				mask = 0x20;
				for(j = 0; j < 6; j++) {
					if(((source[read_posn] - 65) & mask) == 0x00) {
						concat(binary_string, "0");
					} else {
						concat(binary_string, "1");
					}
					mask = mask >> 1;
				}
			}
			
			if(source[read_posn] == '[') {
				concat(binary_string, "11111");
			}
			
			read_posn++;
		} while ((source[read_posn - 1] != '[') && (source[read_posn - 1] != '\0'));
		alpha_pad = 1; /* This is overwritten if a general field is encoded */
	}
	
	if(ai90_mode == 1) {
		/* Alphanumeric mode */
		do {
			if((source[read_posn] >= '0') && (source[read_posn] <= '9')) {
				mask = 0x10;
				for(j = 0; j < 5; j++) {
					if(((source[read_posn] - 43) & mask) == 0x00) {
						concat(binary_string, "0");
					} else {
						concat(binary_string, "1");
					}
					mask = mask >> 1;
				}
			}
			
			if((source[read_posn] >= 'A') && (source[read_posn] <= 'Z')) {
				mask = 0x20;
				for(j = 0; j < 6; j++) {
					if(((source[read_posn] - 33) & mask) == 0x00) {
						concat(binary_string, "0");
					} else {
						concat(binary_string, "1");
					}
					mask = mask >> 1;
				}
			}
			
			switch(source[read_posn]) {
				case '[': concat(binary_string, "01111"); break;
				case '*': concat(binary_string, "111010"); break;
				case ',': concat(binary_string, "111011"); break;
				case '-': concat(binary_string, "111100"); break;
				case '.': concat(binary_string, "111101"); break;
				case '/': concat(binary_string, "111110"); break;
			}
			
			read_posn++;
		} while ((source[read_posn - 1] != '[') && (source[read_posn - 1] != '\0'));
	}

	read_posn += (2 * ai_crop);
	
	/* The compressed data field has been processed if appropriate - the
	rest of the data (if any) goes into a general-purpose data compaction field */
	
	j = 0;
	if(fnc1_latch == 1) {
		/* Encodation method "10" has been used but it is not followed by
		   AI 10, so a FNC1 character needs to be added */
		general_field[j] = '[';
		j++;
	}
	
	for(i = read_posn; i < ustrlen(source); i++) {
		general_field[j] = source[i];
		j++;
	}
	general_field[j] = '\0';
	
	if(strlen(general_field) != 0) { alpha_pad = 0; }
	
	latch = 0;
	for(i = 0; i < strlen(general_field); i++) {
		/* Table 13 - ISO/IEC 646 encodation */
		if((general_field[i] < ' ') || (general_field[i] > 'z')) {
			general_field_type[i] = INVALID_CHAR; latch = 1;
		} else {
			general_field_type[i] = ISOIEC;
		}
		
		if(general_field[i] == '#') {
			general_field_type[i] = INVALID_CHAR; latch = 1;
		}
		if(general_field[i] == '$') {
			general_field_type[i] = INVALID_CHAR; latch = 1;
		}
		if(general_field[i] == '@') {
			general_field_type[i] = INVALID_CHAR; latch = 1;
		}
		if(general_field[i] == 92) {
			general_field_type[i] = INVALID_CHAR; latch = 1;
		}
		if(general_field[i] == '^') {
			general_field_type[i] = INVALID_CHAR; latch = 1;
		}
		if(general_field[i] == 96) {
			general_field_type[i] = INVALID_CHAR; latch = 1;
		}

		/* Table 12 - Alphanumeric encodation */
		if((general_field[i] >= 'A') && (general_field[i] <= 'Z')) {
			general_field_type[i] = ALPHA_OR_ISO;
		}
		if(general_field[i] == '*') {
			general_field_type[i] = ALPHA_OR_ISO;
		}
		if(general_field[i] == ',') {
			general_field_type[i] = ALPHA_OR_ISO;
		}
		if(general_field[i] == '-') {
			general_field_type[i] = ALPHA_OR_ISO;
		}
		if(general_field[i] == '.') {
			general_field_type[i] = ALPHA_OR_ISO;
		}
		if(general_field[i] == '/') {
			general_field_type[i] = ALPHA_OR_ISO;
		}

		/* Numeric encodation */
		if((general_field[i] >= '0') && (general_field[i] <= '9')) {
			general_field_type[i] = ANY_ENC;
		}
		if(general_field[i] == '[') {
			/* FNC1 can be encoded in any system */
			general_field_type[i] = ANY_ENC;
		}

	}
	
	general_field_type[strlen(general_field)] = '\0';
	
	if(latch == 1) {
		/* Invalid characters in input data */
		strcpy(symbol->errtxt, "Invalid characters in input data [A2]");
		return ERROR_INVALID_DATA;
	}

	for(i = 0; i < strlen(general_field); i++) {
		if((general_field_type[i] == ISOIEC) && (general_field[i + 1] == '[')) {
			general_field_type[i + 1] = ISOIEC;
		}
	}
	
	for(i = 0; i < strlen(general_field); i++) {
		if((general_field_type[i] == ALPHA_OR_ISO) && (general_field[i + 1] == '[')) {
			general_field_type[i + 1] = ALPHA_OR_ISO;
		}
	}
	
	latch = general_rules(general_field, general_field_type);
	
	i = 0;
	do {
		switch(general_field_type[i]) {
			case NUMERIC:
				
				if(i != 0) {
					if((general_field_type[i - 1] != NUMERIC) && (general_field[i - 1] != '[')) {
						concat(binary_string, "000"); /* Numeric latch */
					}
				}
				
				if(general_field[i] != '[') {
					d1 = ctoi(general_field[i]);
				} else {
					d1 = 10;
				}
				
				if(general_field[i + 1] != '[') {
					d2 = ctoi(general_field[i + 1]);
				} else {
					d2 = 10;
				}
				
				value = (11 * d1) + d2 + 8;
				
				mask = 0x40;
				for(j = 0; j < 7; j++) {
					if((value & mask) == 0x00) {
						concat(binary_string, "0");
					} else {
						concat(binary_string, "1");
					}
					mask = mask >> 1;
				}
				
				i += 2;
				break;
			
			case ALPHA:
				
				if(i != 0) {
					if((general_field_type[i - 1] == NUMERIC) || (general_field[i - 1] == '[')) {
						concat(binary_string, "0000"); /* Alphanumeric latch */
					}
					if(general_field_type[i - 1] == ISOIEC) {
						concat(binary_string, "00100"); /* ISO/IEC 646 latch */
					}
				}
				
				if((general_field[i] >= '0') && (general_field[i] <= '9')) {
					
					value = general_field[i] - 43;
					
					mask = 0x10;
					for(j = 0; j < 5; j++) {
						if((value & mask) == 0x00) {
							concat(binary_string, "0");
						} else {
							concat(binary_string, "1");
						}
						mask = mask >> 1;
					}
				}
				
				if((general_field[i] >= 'A') && (general_field[i] <= 'Z')) {
					
					value = general_field[i] - 33;
					
					mask = 0x20;
					for(j = 0; j < 6; j++) {
						if((value & mask) == 0x00) {
							concat(binary_string, "0");
						} else {
							concat(binary_string, "1");
						}
						mask = mask >> 1;
					}
				}
				
				if(general_field[i] == '[') concat(binary_string, "01111"); /* FNC1/Numeric latch */
				if(general_field[i] == '*') concat(binary_string, "111010"); /* asterisk */
				if(general_field[i] == ',') concat(binary_string, "111011"); /* comma */
				if(general_field[i] == '-') concat(binary_string, "111100"); /* minus or hyphen */
				if(general_field[i] == '.') concat(binary_string, "111101"); /* period or full stop */
				if(general_field[i] == '/') concat(binary_string, "111110"); /* slash or solidus */
				
				i++;
				break;
			
			case ISOIEC:
				
				if(i != 0) {
					if((general_field_type[i - 1] == NUMERIC) || (general_field[i - 1] == '[')) {
						concat(binary_string, "0000"); /* Alphanumeric latch */
						concat(binary_string, "00100"); /* ISO/IEC 646 latch */
					}
					if(general_field_type[i - 1] == ALPHA) {
						concat(binary_string, "00100"); /* ISO/IEC 646 latch */
					}
				}
				
				if((general_field[i] >= '0') && (general_field[i] <= '9')) {
					
					value = general_field[i] - 43;
					
					mask = 0x10;
					for(j = 0; j < 5; j++) {
						if((value & mask) == 0x00) {
							concat(binary_string, "0");
						} else {
							concat(binary_string, "1");
						}
						mask = mask >> 1;
					}
				}
				
				if((general_field[i] >= 'A') && (general_field[i] <= 'Z')) {
					
					value = general_field[i] - 1;
					
					mask = 0x40;
					for(j = 0; j < 7; j++) {
						if((value & mask) == 0x00) {
							concat(binary_string, "0");
						} else {
							concat(binary_string, "1");
						}
						mask = mask >> 1;
					}
				}
				
				if((general_field[i] >= 'a') && (general_field[i] <= 'z')) {
					
					value = general_field[i] - 7;
					
					mask = 0x40;
					for(j = 0; j < 7; j++) {
						if((value & mask) == 0x00) {
							concat(binary_string, "0");
						} else {
							concat(binary_string, "1");
						}
						mask = mask >> 1;
					}
				}
				
				if(general_field[i] == '[') concat(binary_string, "01111"); /* FNC1/Numeric latch */
				if(general_field[i] == '!') concat(binary_string, "11101000"); /* exclamation mark */
				if(general_field[i] == 34) concat(binary_string, "11101001"); /* quotation mark */
				if(general_field[i] == 37) concat(binary_string, "11101010"); /* percent sign */
				if(general_field[i] == '&') concat(binary_string, "11101011"); /* ampersand */
				if(general_field[i] == 39) concat(binary_string, "11101100"); /* apostrophe */
				if(general_field[i] == '(') concat(binary_string, "11101101"); /* left parenthesis */
				if(general_field[i] == ')') concat(binary_string, "11101110"); /* right parenthesis */
				if(general_field[i] == '*') concat(binary_string, "11101111"); /* asterisk */
				if(general_field[i] == '+') concat(binary_string, "11110000"); /* plus sign */
				if(general_field[i] == ',') concat(binary_string, "11110001"); /* comma */
				if(general_field[i] == '-') concat(binary_string, "11110010"); /* minus or hyphen */
				if(general_field[i] == '.') concat(binary_string, "11110011"); /* period or full stop */
				if(general_field[i] == '/') concat(binary_string, "11110100"); /* slash or solidus */
				if(general_field[i] == ':') concat(binary_string, "11110101"); /* colon */
				if(general_field[i] == ';') concat(binary_string, "11110110"); /* semicolon */
				if(general_field[i] == '<') concat(binary_string, "11110111"); /* less-than sign */
				if(general_field[i] == '=') concat(binary_string, "11111000"); /* equals sign */
				if(general_field[i] == '>') concat(binary_string, "11111001"); /* greater-than sign */
				if(general_field[i] == '?') concat(binary_string, "11111010"); /* question mark */
				if(general_field[i] == '_') concat(binary_string, "11111011"); /* underline or low line */
				if(general_field[i] == ' ') concat(binary_string, "11111100"); /* space */
				
				i++;
				break;
		}
	} while (i + latch < strlen(general_field));

	binary_length = strlen(binary_string);
	if(cc_mode == 1) {
		/* CC-A 2D component - calculate remaining space */
		switch(*(cc_width)) {
			case 2:
				if(binary_length > 167) { return ERROR_TOO_LONG; }
				if(binary_length <= 167) { target_bitsize = 167; }
				if(binary_length <= 138) { target_bitsize = 138; }
				if(binary_length <= 118) { target_bitsize = 118; }
				if(binary_length <= 108) { target_bitsize = 108; }
				if(binary_length <= 88) { target_bitsize = 88; }
				if(binary_length <= 78) { target_bitsize = 78; }
				if(binary_length <= 59) { target_bitsize = 59; }
				break;
			case 3:
				if(binary_length > 167) { return ERROR_TOO_LONG; }
				if(binary_length <= 167) { target_bitsize = 167; }
				if(binary_length <= 138) { target_bitsize = 138; }
				if(binary_length <= 118) { target_bitsize = 118; }
				if(binary_length <= 98) { target_bitsize = 98; }
				if(binary_length <= 78) { target_bitsize = 78; }
				break;
			case 4:
				if(binary_length > 197) { return ERROR_TOO_LONG; }
				if(binary_length <= 197) { target_bitsize = 197; }
				if(binary_length <= 167) { target_bitsize = 167; }
				if(binary_length <= 138) { target_bitsize = 138; }
				if(binary_length <= 108) { target_bitsize = 108; }
				if(binary_length <= 78) { target_bitsize = 78; }
				break;
		}
	}
	
	if(cc_mode == 2) {
		/* CC-B 2D component - calculated from ISO/IEC 24728 Table 1  */
		switch(*(cc_width)) {
			case 2:
				if(binary_length > 336) { return ERROR_TOO_LONG; }
				if(binary_length <= 336) { target_bitsize = 336; }
				if(binary_length <= 296) { target_bitsize = 296; }
				if(binary_length <= 256) { target_bitsize = 256; }
				if(binary_length <= 208) { target_bitsize = 208; }
				if(binary_length <= 160) { target_bitsize = 160; }
				if(binary_length <= 104) { target_bitsize = 104; }
				if(binary_length <= 56) { target_bitsize = 56; }
				break;
			case 3:
				if(binary_length > 768) { return ERROR_TOO_LONG; }
				if(binary_length <= 768) { target_bitsize = 768; }
				if(binary_length <= 648) { target_bitsize = 648; }
				if(binary_length <= 536) { target_bitsize = 536; }
				if(binary_length <= 416) { target_bitsize = 416; }
				if(binary_length <= 304) { target_bitsize = 304; }
				if(binary_length <= 208) { target_bitsize = 208; }
				if(binary_length <= 152) { target_bitsize = 152; }
				if(binary_length <= 112) { target_bitsize = 112; }
				if(binary_length <= 72) { target_bitsize = 72; }
				if(binary_length <= 32) { target_bitsize = 32; }
				break;
			case 4:
				if(binary_length > 1184) { return ERROR_TOO_LONG; }
				if(binary_length <= 1184) { target_bitsize = 1184; }
				if(binary_length <= 1016) { target_bitsize = 1016; }
				if(binary_length <= 840) { target_bitsize = 840; }
				if(binary_length <= 672) { target_bitsize = 672; }
				if(binary_length <= 496) { target_bitsize = 496; }
				if(binary_length <= 352) { target_bitsize = 352; }
				if(binary_length <= 264) { target_bitsize = 264; }
				if(binary_length <= 208) { target_bitsize = 208; }
				if(binary_length <= 152) { target_bitsize = 152; }
				if(binary_length <= 96) { target_bitsize = 96; }
				if(binary_length <= 56) { target_bitsize = 56; }
				break;
		}
	}
	
	if (cc_mode == 3) {
		/* CC-C 2D Component is a bit more complex! */
		int byte_length, codewords_used, ecc_level, ecc_codewords, rows;
		int codewords_total, target_codewords, target_bytesize;
		
		byte_length = binary_length / 8;
		if(binary_length % 8 != 0) { byte_length++; }
		
		codewords_used = (byte_length / 6) * 5;
		codewords_used += byte_length % 6;
		
		ecc_level = 7;
		if(codewords_used <= 1280) { ecc_level = 6; }
		if(codewords_used <= 640) { ecc_level = 5; }
		if(codewords_used <= 320) { ecc_level = 4; }
		if(codewords_used <= 160) { ecc_level = 3; }
		if(codewords_used <= 40) { ecc_level = 2; }
		*(ecc) = ecc_level;
		ecc_codewords = 1;
		for(i = 1; i <= (ecc_level + 1); i++){
			ecc_codewords *= 2;
		}
		
		codewords_used += ecc_codewords;
		codewords_used += 3;
		
		if(codewords_used > symbol->option_3) {
			return ERROR_TOO_LONG;
		}
		/* *(cc_width) = 0.5 + sqrt((codewords_used) / 3); */
		*(cc_width) = (lin_width - 62) / 17;
		if((codewords_used / *(cc_width)) > 90) {
			/* stop the symbol from becoming too high */
			*(cc_width) = *(cc_width) + 1;
		}
		
		rows = codewords_used / *(cc_width);
		if(codewords_used % *(cc_width) != 0) {
			rows++;
		}
		
		codewords_total = *(cc_width) * rows;
		
		target_codewords = codewords_total - ecc_codewords;
		target_codewords -= 3;
		
		target_bytesize = 6 * (target_codewords / 5);
		target_bytesize += target_codewords % 5;
		
		target_bitsize = 8 * target_bytesize;
	}
	
	remainder = binary_length - target_bitsize;
	
	if(latch == 1) {
		i = 0;
		/* There is still one more numeric digit to encode */
		
		if((remainder >= 4) && (remainder <= 6)) {
			d1 = ctoi(general_field[i]);
			d1++;
			
			mask = 0x08;
			for(j = 0; j < 4; j++) {
				if((value & mask) == 0x00) {
					concat(binary_string, "0");
				} else {
					concat(binary_string, "1");
				}
				mask = mask >> 1;
			}
		} else {
			d1 = ctoi(general_field[i]);
			d2 = 10;
				
			value = (11 * d1) + d2 + 8;
				
			mask = 0x40;
			for(j = 0; j < 7; j++) {
				if((value & mask) == 0x00) {
					concat(binary_string, "0");
				} else {
					concat(binary_string, "1");
				}
				mask = mask >> 1;
			}
			/* This may push the symbol up to the next size */
		}
	}
	
	if(strlen(binary_string) > 11805) { /* (2361 * 5) */
		strcpy(symbol->errtxt, "Input too long [A3]");
		return ERROR_TOO_LONG;
	}
	
	/* all the code below is repeated from above - it needs to be calculated again because the
	   size of the symbol may have changed when adding data in the above sequence */
	
	binary_length = strlen(binary_string);
	if(cc_mode == 1) {
		/* CC-A 2D component - calculate padding required */
		switch(*(cc_width)) {
			case 2:
				if(binary_length > 167) { return ERROR_TOO_LONG; }
				if(binary_length <= 167) { target_bitsize = 167; }
				if(binary_length <= 138) { target_bitsize = 138; }
				if(binary_length <= 118) { target_bitsize = 118; }
				if(binary_length <= 108) { target_bitsize = 108; }
				if(binary_length <= 88) { target_bitsize = 88; }
				if(binary_length <= 78) { target_bitsize = 78; }
				if(binary_length <= 59) { target_bitsize = 59; }
				break;
			case 3:
				if(binary_length > 167) { return ERROR_TOO_LONG; }
				if(binary_length <= 167) { target_bitsize = 167; }
				if(binary_length <= 138) { target_bitsize = 138; }
				if(binary_length <= 118) { target_bitsize = 118; }
				if(binary_length <= 98) { target_bitsize = 98; }
				if(binary_length <= 78) { target_bitsize = 78; }
				break;
			case 4:
				if(binary_length > 197) { return ERROR_TOO_LONG; }
				if(binary_length <= 197) { target_bitsize = 197; }
				if(binary_length <= 167) { target_bitsize = 167; }
				if(binary_length <= 138) { target_bitsize = 138; }
				if(binary_length <= 108) { target_bitsize = 108; }
				if(binary_length <= 78) { target_bitsize = 78; }
				break;
		}
	}
	
	if(cc_mode == 2) {
		/* CC-B 2D component */
		switch(*(cc_width)) {
			case 2:
				if(binary_length > 336) { return ERROR_TOO_LONG; }
				if(binary_length <= 336) { target_bitsize = 336; }
				if(binary_length <= 296) { target_bitsize = 296; }
				if(binary_length <= 256) { target_bitsize = 256; }
				if(binary_length <= 208) { target_bitsize = 208; }
				if(binary_length <= 160) { target_bitsize = 160; }
				if(binary_length <= 104) { target_bitsize = 104; }
				if(binary_length <= 56) { target_bitsize = 56; }
				break;
			case 3:
				if(binary_length > 768) { return ERROR_TOO_LONG; }
				if(binary_length <= 768) { target_bitsize = 768; }
				if(binary_length <= 648) { target_bitsize = 648; }
				if(binary_length <= 536) { target_bitsize = 536; }
				if(binary_length <= 416) { target_bitsize = 416; }
				if(binary_length <= 304) { target_bitsize = 304; }
				if(binary_length <= 208) { target_bitsize = 208; }
				if(binary_length <= 152) { target_bitsize = 152; }
				if(binary_length <= 112) { target_bitsize = 112; }
				if(binary_length <= 72) { target_bitsize = 72; }
				if(binary_length <= 32) { target_bitsize = 32; }
				break;
			case 4:
				if(binary_length > 1184) { return ERROR_TOO_LONG; }
				if(binary_length <= 1184) { target_bitsize = 1184; }
				if(binary_length <= 1016) { target_bitsize = 1016; }
				if(binary_length <= 840) { target_bitsize = 840; }
				if(binary_length <= 672) { target_bitsize = 672; }
				if(binary_length <= 496) { target_bitsize = 496; }
				if(binary_length <= 352) { target_bitsize = 352; }
				if(binary_length <= 264) { target_bitsize = 264; }
				if(binary_length <= 208) { target_bitsize = 208; }
				if(binary_length <= 152) { target_bitsize = 152; }
				if(binary_length <= 96) { target_bitsize = 96; }
				if(binary_length <= 56) { target_bitsize = 56; }
				break;
		}
	}
	
	if (cc_mode == 3) {
		/* CC-C 2D Component is a bit more complex! */
		int byte_length, codewords_used, ecc_level, ecc_codewords, rows;
		int codewords_total, target_codewords, target_bytesize;
		
		byte_length = binary_length / 8;
		if(binary_length % 8 != 0) { byte_length++; }
		
		codewords_used = (byte_length / 6) * 5;
		codewords_used += byte_length % 6;
		
		ecc_level = 7;
		if(codewords_used <= 1280) { ecc_level = 6; }
		if(codewords_used <= 640) { ecc_level = 5; }
		if(codewords_used <= 320) { ecc_level = 4; }
		if(codewords_used <= 160) { ecc_level = 3; }
		if(codewords_used <= 40) { ecc_level = 2; }
		*(ecc) = ecc_level;
		ecc_codewords = 1;
		for(i = 1; i <= (ecc_level + 1); i++){
			ecc_codewords *= 2;
		}
		
		codewords_used += ecc_codewords;
		codewords_used += 3;
		
		if(codewords_used > symbol->option_3) {
			return ERROR_TOO_LONG;
		}
		/* *(cc_width) = 0.5 + sqrt((codewords_used) / 3); */
		*(cc_width) = (lin_width - 62) / 17;
		if((codewords_used / *(cc_width)) > 90) {
			/* stop the symbol from becoming too high */
			*(cc_width) = *(cc_width) + 1;
		}
		
		rows = codewords_used / *(cc_width);
		if(codewords_used % *(cc_width) != 0) {
			rows++;
		}
		
		codewords_total = *(cc_width) * rows;
		
		target_codewords = codewords_total - ecc_codewords;
		target_codewords -= 3;
		
		target_bytesize = 6 * (target_codewords / 5);
		target_bytesize += target_codewords % 5;
		
		target_bitsize = 8 * target_bytesize;
	}
	
	if(binary_length < target_bitsize) {
		/* Now add padding to binary string */
		if (alpha_pad == 1) {
			concat(binary_string, "11111");
			alpha_pad = 0;
			/* Extra FNC1 character required after Alpha encodation (section 5.2.3) */
		}
		
		if ((strlen(general_field) != 0) && (general_field_type[strlen(general_field) - 1] == NUMERIC)) {
			concat(binary_string, "0000");
		}

		while (strlen(binary_string) < target_bitsize) {
			concat(binary_string, "00100");
		}
		
		if(strlen(binary_string) > target_bitsize) {
			binary_string[target_bitsize] = '\0';
		}
	}
	
	return 0;
}

int composite(struct zint_symbol *symbol, unsigned char source[])
{
	int errno, cc_mode, cc_width, ecc_level;
	int j, last_ai, ai_latch, i, k, separator_row;
	unsigned char reduced[3000];
	char binary_string[10 * ustrlen(source)], ai_string[4];
	struct zint_symbol *linear;
	int top_shift, bottom_shift;
	
	errno = 0;
	separator_row = 0;
	
	if(strlen(symbol->primary) == 0) {
		strcpy(symbol->errtxt, "No primary (linear) message in 2D composite [A4]");
		return ERROR_INVALID_OPTION;
	}
	
	if(ustrlen(source) > 2990) {
		strcpy(symbol->errtxt, "2D component input data too long [A5]");
		return ERROR_TOO_LONG;
	}
	
	if(source[0] != '[') {
		strcpy(symbol->errtxt, "Data does not start with an AI [A6]");
		return ERROR_INVALID_DATA;
	}
	
	for(i = 0; i < ustrlen(source) - 1; i++) {
		if((source[i] == '[') && (source[i + 1] == '[')) {
			/* Can't have nested brackets - Quit */
			strcpy(symbol->errtxt, "Nested AI detected (two or more open brackets) [A7]");
			return ERROR_INVALID_DATA;
		}
	}
	
	for(i = 0; i < ustrlen(source) - 1; i++) {
		if((source[i] == ']') && (source[i + 1] == ']')) {
			/* Can't have nested brackets - Quit */
			strcpy(symbol->errtxt, "Nested AI detected (two or more close brackets) [A8]");
			return ERROR_INVALID_DATA;
		}
	}
	
	linear = ZBarcode_Create(); /* Symbol contains the 2D component and Linear contains the rest */
	
	/* Resolve AI data - put resulting string in 'reduced' */
	j = 0;
	last_ai = 0;
	ai_latch = 1;
	for(i = 0; i < ustrlen(source); i++) {
		if((source[i] != '[') && (source[i] != ']')) {
			reduced[j] = source[i];
			j++;
		}
		if(source[i] == '[') {
			/* Start of an AI string */
			if(ai_latch == 0) {
				reduced[j] = '[';
				j++;
			}
			ai_string[0] = source[i + 1];
			ai_string[1] = source[i + 2];
			ai_string[2] = '\0';
			last_ai = atoi(ai_string);
			ai_latch = 0;
			/* The following values from GS1 specification figure 5.3.8.2.1 - 1
			"Element Strings with Pre-Defined Length Using Application Identifiers" */
			if((last_ai >= 0) && (last_ai <= 4)) { ai_latch = 1; }
			if((last_ai >= 11) && (last_ai <= 20)) { ai_latch = 1; }
			if(last_ai == 23) { ai_latch = 1; } /* legacy support - see 5.3.8.2.2 */
			if((last_ai >= 31) && (last_ai <= 36)) { ai_latch = 1; }
			if(last_ai == 41) { ai_latch = 1; }
		}
		/* The ']' character is simply dropped from the input */
	}
	reduced[j] = '\0';
	
	/* Note that no attempt is made to verify that the data to be encoded does
	actually conform to the right data length - that is required of the person or
	program inputting the data */
	
	cc_mode = symbol->option_1;
	
	if((cc_mode == 3) && (symbol->symbology != BARCODE_EAN128_CC)) {
		/* CC-C can only be used with a GS1-128 linear part */
		strcpy(symbol->errtxt, "Invalid mode (CC-C only valid with GS1-128 linear component) [A9]");
		return ERROR_INVALID_OPTION;
	}
	
	linear->symbology = symbol->symbology;
	
	if(linear->symbology != BARCODE_EAN128_CC) {
		/* Set the "component linkage" flag in the linear component */
		linear->option_1 = 2;
	} else {
		/* GS1-128 needs to know which type of 2D component is used */
		linear->option_1 = cc_mode;
	}
	
	switch(symbol->symbology) {
		case BARCODE_EANX_CC: errno = eanx(linear, (unsigned char *)symbol->primary); break;
		case BARCODE_EAN128_CC: errno = ean_128(linear, (unsigned char *)symbol->primary); break;
		case BARCODE_RSS14_CC: errno = rss14(linear, (unsigned char *)symbol->primary); break;
		case BARCODE_RSS_LTD_CC: errno = rsslimited(linear, (unsigned char *)symbol->primary); break;
		case BARCODE_RSS_EXP_CC: errno = rssexpanded(linear, (unsigned char *)symbol->primary); break;
		case BARCODE_UPCA_CC: errno = eanx(linear, (unsigned char *)symbol->primary); break;
		case BARCODE_UPCE_CC: errno = eanx(linear, (unsigned char *)symbol->primary); break;
		case BARCODE_RSS14STACK_CC: errno = rss14(linear, (unsigned char *)symbol->primary); break;
		case BARCODE_RSS14_OMNI_CC: errno = rss14(linear, (unsigned char *)symbol->primary); break;
		case BARCODE_RSS_EXPSTACK_CC: errno = rssexpanded(linear, (unsigned char *)symbol->primary); break;
	}
	
	switch(symbol->symbology) {
		/* Determine width of 2D component according to ISO/IEC 24723 Table 1 */
		case BARCODE_EANX_CC:
			switch(strlen(symbol->primary)) {
				case 7: /* EAN-8 */
				case 10: /* EAN-8 + 2 */
				case 13: /* EAN-8 + 5 */
					cc_width = 3;
					break;
				case 12: /* EAN-13 */
				case 15: /* EAN-13 + 2 */
				case 18: /* EAN-13 + 5 */
					cc_width = 4;
					break;
			}
			break;
		case BARCODE_EAN128_CC: cc_width = 4; break;
		case BARCODE_RSS14_CC: cc_width = 4; break;
		case BARCODE_RSS_LTD_CC: cc_width = 3; break;
		case BARCODE_RSS_EXP_CC: cc_width = 4; break;
		case BARCODE_UPCA_CC: cc_width = 4; break;
		case BARCODE_UPCE_CC: cc_width = 2; break;
		case BARCODE_RSS14STACK_CC: cc_width = 2; break;
		case BARCODE_RSS14_OMNI_CC: cc_width = 2; break;
		case BARCODE_RSS_EXPSTACK_CC: cc_width = 4; break;
	}
	
	strcpy(binary_string, "");
	
	if(errno != 0) {
		return errno;
	}
	
	if(cc_mode == 1) {
		i = cc_binary_string(symbol, reduced, binary_string, cc_mode, &cc_width, &ecc_level, linear->width);
		if (i == ERROR_TOO_LONG) {
			cc_mode = 2;
		}
	}
	
	if(cc_mode == 2) { /* If the data didn't fit into CC-A it is recalculated for CC-B */
		i = cc_binary_string(symbol, reduced, binary_string, cc_mode, &cc_width, &ecc_level, linear->width);
		if (i == ERROR_TOO_LONG) {
			if(symbol->symbology != BARCODE_EAN128_CC) {
				return ERROR_TOO_LONG;
			} else {
				cc_mode = 3;
			}
		}
	}
	
	if(cc_mode == 3) { /* If the data didn't fit in CC-B (and linear part is GS1-128) it is recalculated
				for CC-C */
		i = cc_binary_string(symbol, reduced, binary_string, cc_mode, &cc_width, &ecc_level, linear->width);
		if (i == ERROR_TOO_LONG) {
			return ERROR_TOO_LONG;
		}
	}
	
	switch(cc_mode) { /* Note that ecc_level is only relevant to CC-C */
		case 1: errno = cc_a(symbol, (unsigned char*)binary_string, cc_width); break;
		case 2: errno = cc_b(symbol, (unsigned char*)binary_string, cc_width); break;
		case 3: errno = cc_c(symbol, (unsigned char*)binary_string, cc_width, ecc_level); break;
	}
	
	if(errno != 0) {
		return ERROR_ENCODING_PROBLEM;
	}
	
	/* Merge the linear component with the 2D component */
	
	top_shift = 0;
	bottom_shift = 0;
	
	switch(symbol->symbology) {
		/* Determine horizontal alignment (according to section 12.3) */
		case BARCODE_EANX_CC:
			switch(strlen(symbol->primary)) {
				case 7: /* EAN-8 */
				case 10: /* EAN-8 + 2 */
				case 13: /* EAN-8 + 5 */
					bottom_shift = 13;
					break;
				case 12: /* EAN-13 */
				case 15: /* EAN-13 + 2 */
				case 18: /* EAN-13 + 5 */
					bottom_shift = 2;
					break;
			}
			break;
		case BARCODE_EAN128_CC: if(cc_mode == 3) {
				bottom_shift = 7;
			}
			break;
		case BARCODE_RSS14_CC: bottom_shift = 4; break;
		case BARCODE_RSS_LTD_CC: bottom_shift = 9; break;
		case BARCODE_RSS_EXP_CC: k = 1;
			while((linear->encoded_data[1][k - 1] != '1') && (linear->encoded_data[1][k] != '0')) {
				k++;
			}
			top_shift = k;
			break;
		case BARCODE_UPCA_CC: bottom_shift = 2; break;
		case BARCODE_UPCE_CC: bottom_shift = 2; break;
		case BARCODE_RSS14STACK_CC: top_shift = 1; break;
		case BARCODE_RSS14_OMNI_CC: top_shift = 1; break;
		case BARCODE_RSS_EXPSTACK_CC: k = 1;
			while((linear->encoded_data[1][k - 1] != '1') && (linear->encoded_data[1][k] != '0')) {
				k++;
			}
			top_shift = k;
			break;
	}
	
	if(top_shift != 0) {
		for(i = 0; i <= symbol->rows; i++) {
			for(j = (symbol->width + top_shift); j >= top_shift; j--) {
				symbol->encoded_data[i][j] = symbol->encoded_data[i][j - top_shift];
			}
			for(j = 0; j < top_shift; j++) {
				symbol->encoded_data[i][j] = 0;
			}
		}
	} 
	
	for(i = 0; i <= linear->rows; i++) {
		symbol->row_height[symbol->rows + i] = linear->row_height[i];
		for(j = 0; j <= linear->width; j++) {
			symbol->encoded_data[i + symbol->rows][j + bottom_shift] = linear->encoded_data[i][j];
		}
	}
	if((linear->width + bottom_shift) > symbol->width) {
		symbol->width = linear->width + bottom_shift;
	}
	if((symbol->width + top_shift) > symbol->width) {
		symbol->width += top_shift;
	}
	symbol->rows += linear->rows;
	strcpy(symbol->text, linear->text);

	
	ZBarcode_Delete(linear);
	
	return errno;
}
