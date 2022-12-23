/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2021 Steffen Wendzel <wendzel (at) hs-worms (dot) de>
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

extern unsigned short use_tls; /* config.y */
extern unsigned short tls_mutual_auth; /* config.y */
extern int tls_port; /* config.y */
extern char *tls_ca_file; /* config.y */
extern char *tls_cert_file; /* config.y */
extern char *tls_key_file; /* config.y */
extern char *tls_crl_file; /* config.y */
extern char *tls_ciper_prio; /* config.y */
extern char *tls_enabled_versions; /* config.y */
extern const char default_tls_versions[]; /* config.y */
// extern const char default_cipher_prio[];

gnutls_certificate_credentials_t x509_credentials;

int
tls_init()
{
  int return_code;

  return_code = gnutls_global_init();
  if (return_code != GNUTLS_E_SUCCESS) {
    DO_SYSL("TLS not initialized, gnutls_global_init() error");

    fprintf(stderr, "TLS not initialized, gnutls_global_init() error\n");
    fprintf(stderr, "%s\n", gnutls_strerror(return_code));
    return FALSE;
  }

  return_code = gnutls_certificate_allocate_credentials(&x509_credentials);
  if (return_code != GNUTLS_E_SUCCESS) {
    DO_SYSL("TLS not initialized, could not allocate GnuTLS credentials");

    fprintf(stderr, "TLS not initialized, could not allocate GnuTLS credentials\”");
    fprintf(stderr, "%s\n", gnutls_strerror(return_code));
    return FALSE;
  }

  /* returns the number of processed certs, or an error */
  return_code = gnutls_certificate_set_x509_trust_file(x509_credentials, tls_ca_file, GNUTLS_X509_FMT_PEM);
  if (return_code < 0) {
    DO_SYSL("TLS not initialized, could not set CA file");

    fprintf(stderr, "TLS not initialized, could not set CA file\n");
    fprintf(stderr, "%s\n", gnutls_strerror(return_code));
    return FALSE;
  }

  if (tls_crl_file) {
    return_code = gnutls_certificate_set_x509_crl_file(x509_credentials, tls_crl_file, GNUTLS_X509_FMT_PEM);
    if (return_code < 0) {
      DO_SYSL("TLS not initialized, could not set CRL file");

      fprintf(stderr, "TLS not initialized, could not set CRL file\n");
      fprintf(stderr, "%s\n", gnutls_strerror(return_code));
    return FALSE;
    }
  }

  return_code = gnutls_certificate_set_x509_key_file(x509_credentials, tls_cert_file, tls_key_file, GNUTLS_X509_FMT_PEM);
  if (return_code != GNUTLS_E_SUCCESS) {
    DO_SYSL("TLS not initialized, could not set cert or key file");

    fprintf(stderr, "TLS not initialized, could not set cert or key file\n");
    fprintf(stderr, "%s\n", gnutls_strerror(return_code));
    return FALSE;
  }

  /* enable only specified TLS versions */
  /* must be done befor the prio string */
  if (!tls_enabled_versions) {
    if (!(tls_enabled_versions = strdup(default_tls_versions))) {
      DO_SYSL("strdup() error while getting default TLS enabled version")
    }
  }
  if (tls_enabled_versions) {
    /* disable all if we want to use custom versions */
    gnutls_protocol_set_enabled(GNUTLS_TLS1_0, 0);
    gnutls_protocol_set_enabled(GNUTLS_TLS1_1, 0);
    gnutls_protocol_set_enabled(GNUTLS_TLS1_2, 0);
    gnutls_protocol_set_enabled(GNUTLS_TLS1_3, 0);

    char delimeter[] = ":";
    char *ptr;

    ptr = strtok(tls_enabled_versions, delimeter);

    while (ptr != NULL) {
      if (strncasecmp(ptr, "TLS1.0", 6) == 0) {
        gnutls_protocol_set_enabled(GNUTLS_TLS1_0, 1);
        DO_SYSL("Enabled TLS 1.0 - NOT RECOMMENDED");
      } else if (strncasecmp(ptr, "TLS1.1", 6) == 0) {
        gnutls_protocol_set_enabled(GNUTLS_TLS1_1, 1);
        DO_SYSL("Enabled TLS 1.1 - NOT RECOMMENDED");
      } else if (strncasecmp(ptr, "TLS1.2", 6) == 0) {
        gnutls_protocol_set_enabled(GNUTLS_TLS1_2, 1);
        DO_SYSL("Enabled TLS 1.2");
      } else if (strncasecmp(ptr, "TLS1.3", 6) == 0) {
        gnutls_protocol_set_enabled(GNUTLS_TLS1_3, 1);
        DO_SYSL("Enabled TLS 1.3");
      } else {
        fprintf(stderr, "Unknown TLS version %s\n", ptr);
        DO_SYSL("Unknown TLS version in tls-versions");
      }
      ptr = strtok(NULL, delimeter);
    }
  }
  ///gnutls_protocol_set_enabled()

  // gnutls_priority_init()

  DO_SYSL("TLS initialized");
  return TRUE;
}

int
tls_close()
{
  DO_SYSL("TLS connection closed");
  return TRUE;
}
