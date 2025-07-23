/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2017 Steffen Wendzel <steffen (at) wendzel (dot) de>
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
/* VERSION is defined by mhash.h (from libmhash) too */
#undef VERSION
#include <mhash.h>

#define SHA256_LEN 64 /* 32 bytes * 8 bits = 256 bits;
		       * however, coded as 4-bit hex-tuples makes 64 Bytes */

void bzero_and_free_sensitive_strings(char *str_1, char *str_2, char *str_3)
{
	if (str_1) { bzero(str_1, strlen(str_1)); free(str_1); }
	if (str_2) { bzero(str_2, strlen(str_2)); free(str_2); }
	if (str_3) { bzero(str_3, strlen(str_3)); free(str_3); }
}

char *get_sha256_hash_from_str(char *username /* don't free! */, char *password)
{
	int i;
	MHASH td;
	char *hash = NULL;
	unsigned char *hash_raw = NULL;
	char *strs_plus_salt = NULL;
	extern char *hash_salt; /* from configuration file */
	int len_strs_plus_salt;

	/* because we allocate only half the bytes for the binary value but
	 * all the bytes for the hex value, see below */
	assert(SHA256_LEN % 2 == 0);
	if (!(hash = calloc(SHA256_LEN + 1, 1)))
		return NULL;

	if (!(hash_raw = calloc(SHA256_LEN + 1, 1)))
		return NULL;

	len_strs_plus_salt = strlen(username) + strlen(password) + strlen(hash_salt);
	if (!(strs_plus_salt = calloc(len_strs_plus_salt + 1, 1)))
		return NULL;

	bzero(hash, SHA256_LEN + 1);
	bzero(hash_raw, SHA256_LEN + 1);
	bzero(strs_plus_salt, len_strs_plus_salt + 1);

	/* combine salt (essentially [hash_salt||username] to prevent pw-identification attacks) and password */
	snprintf(strs_plus_salt, len_strs_plus_salt + 1, "%s%s%s", hash_salt, username, password);

	if ((td = mhash_init(MHASH_SHA256)) == MHASH_FAILED) {
		bzero_and_free_sensitive_strings(hash, (char*) hash_raw, strs_plus_salt); /* overwrite with zeros */
		DO_SYSL("mhash_init() returned MHASH_FAILED. Aborting connection.")
		return NULL;
	}

	/* mhash() always returns MUTILS_OK in the library's code, i.e. no
	 * error checking necessary. */
	mhash(td, strs_plus_salt, len_strs_plus_salt);
	/* mhash_deinit() returns void, so no checking here either */
	mhash_deinit(td, hash_raw);

	for (i = 0; i < SHA256_LEN/2; i++) {
		snprintf(hash + (2 * i), 4, "%.2x", hash_raw[i]);
	}
	bzero_and_free_sensitive_strings((char*) hash_raw, strs_plus_salt, NULL); /* overwrite with zeros */
	return hash;
}

/*int main()
{
	printf("%s\n", get_sha256_hash_from_str("Katze\0"));
	return 0;
}
*/
