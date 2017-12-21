/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2017 Steffen Wendzel <wendzel (at) hs-worms (dot) de>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "main.h"
#include <mhash.h>

#define SHA256_LEN 64 /* 32 bytes * 8 bits = 256 bits;
		       * however, coded as 4-bit hex-tuples makes 64 Bytes */

char *get_sha256_hash_from_str(char *str)
{
	int i;
	MHASH td;
	char *hash = NULL;
	char *hash_raw = NULL;
	
	/* because we allocate only half the bytes for the binary value but
	 * all the bytes for the hex value, see below */
	assert(SHA256_LEN % 2 == 0);
	if (!(hash = calloc(SHA256_LEN + 1, 1)))
		return NULL;
	
	if (!(hash_raw = calloc(SHA256_LEN + 1, 1)))
		return NULL;
	
	bzero(hash, SHA256_LEN + 1);
	bzero(hash_raw, SHA256_LEN + 1);
	
	if ((td = mhash_init(MHASH_SHA256)) == MHASH_FAILED) {
		//free(hash); free(hash_raw);
		DO_SYSL("mhash_init() returned MHASH_FAILED. Aborting connection.")
		return NULL;
	}
	
	/* mhash() always returns MUTILS_OK in the library's code, i.e. no
	 * error checking necessary. */
	mhash(td, str, strlen(str));
	/* mhash_deinit() returns void, so no checking here either */
	mhash_deinit(td, hash_raw);
	
	for (i = 0; i < SHA256_LEN/2; i++) {
		snprintf(hash + (2 * i), 4, "%.2x", hash_raw[i]);
	}
	free(hash_raw);
	return hash;
}

/*int main()
{
	printf("%s\n", get_sha256_hash_from_str("Katze\0"));
	return 0;
}
*/

