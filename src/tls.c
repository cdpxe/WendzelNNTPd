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
#include <gnutls/gnutls.h>

extern unsigned short use_tls; /* config.y */
extern int tls_port; /* config.y */
extern char *tls_ca_file; /* config.y */
extern char *tls_cert_file; /* config.y */
extern char *tls_key_file; /* config.y */

static gnutls_certificate_credentials_t x509_credentials;

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

  return_code = gnutls_certificate_set_x509_key_file(x509_credentials, tls_cert_file, tls_key_file, GNUTLS_X509_FMT_PEM);
  if (return_code != GNUTLS_E_SUCCESS) {
    DO_SYSL("TLS not initialized, could not set cert or key file");

    fprintf(stderr, "TLS not initialized, could not set cert or key file\n");
    fprintf(stderr, "%s\n", gnutls_strerror(return_code));
    return FALSE;
  }

  DO_SYSL("TLS initialized");
  return TRUE;
}

int
tls_close()
{
  DO_SYSL("TLS connection closed");
  return TRUE;
}
