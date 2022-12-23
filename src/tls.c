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
// extern const char default_cipher_prio[];

gnutls_certificate_credentials_t x509_credentials;
gnutls_priority_t tls_cipher_priorities;


int
tls_global_init()
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

    fprintf(stderr, "TLS not initialized, could not allocate GnuTLS credentials\n");
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

#if GNUTLS_VERSION_NUMBER >= 0x030506
  /* only available since GnuTLS 3.5.6, on previous versions see */
  return_code = gnutls_certificate_set_known_dh_params(x509_credentials, GNUTLS_SEC_PARAM_MEDIUM);
  if (return_code != GNUTLS_E_SUCCESS) {
    DO_SYSL("TLS not initialized, could not set DH parameters");

    fprintf(stderr, "TLS not initialized, could not set DH parameters\n");
    fprintf(stderr, "%s\n", gnutls_strerror(return_code));
    return FALSE;
  }
#endif

  return_code = gnutls_priority_init(&tls_cipher_priorities, NULL, NULL);
  if (return_code != GNUTLS_E_SUCCESS) {
    DO_SYSL("TLS not initialized, could not set cipher priorities");

    fprintf(stderr, "TLS not initialized, could not set cipher prioritires\n");
    fprintf(stderr, "%s\n", gnutls_strerror(return_code));
    return FALSE;
  }


  DO_SYSL("TLS initialized");
  return TRUE;
}

int
tls_session_init(gnutls_session_t *session, int sockfd)
{
  int return_code;

  return_code = gnutls_init(session, GNUTLS_SERVER);
  if (return_code != GNUTLS_E_SUCCESS) {
    DO_SYSL("TLS session not initialized, gnutls_init() error");

    fprintf(stderr, "TLS sessionnot initialized, gnutls_init() error\n");
    fprintf(stderr, "%s\n", gnutls_strerror(return_code));
    return FALSE;
  }

  return_code = gnutls_priority_set(*session, tls_cipher_priorities);
  if (return_code != GNUTLS_E_SUCCESS) {
    DO_SYSL("TLS session not initialized, could not set cipher priorities");

    fprintf(stderr, "TLS session not initialized, could not set cipher priorities\n");
    fprintf(stderr, "%s\n", gnutls_strerror(return_code));
    return FALSE;
  }

  return_code = gnutls_credentials_set(*session, GNUTLS_CRD_CERTIFICATE, x509_credentials);
  if (return_code != GNUTLS_E_SUCCESS) {
    DO_SYSL("TLS session not initialized, could not set credential certificate");

    fprintf(stderr, "TLS sessionnot initialized, could not set credential certificate\n");
    fprintf(stderr, "%s\n", gnutls_strerror(return_code));
    return FALSE;
  }

  /* XXX mutual TLS here */
  gnutls_certificate_server_set_request(*session, GNUTLS_CERT_IGNORE);
  gnutls_handshake_set_timeout(*session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);

  gnutls_transport_set_int(*session, sockfd);

  do {
    return_code = gnutls_handshake(*session);
  } while (return_code == GNUTLS_E_AGAIN || return_code == GNUTLS_E_INTERRUPTED);

  if (return_code < 0) {
    DO_SYSL("TLS session not initialized, error during handshake");

    fprintf(stderr, "TLS sessionnot initialized, error during handshake\n");
    fprintf(stderr, "%s\n", gnutls_strerror(return_code));

    gnutls_deinit(*session);
    return FALSE;
  }

  return TRUE;
}

void
tls_session_close(gnutls_session_t session)
{
  int return_code;

  do {
    return_code = gnutls_bye(session, GNUTLS_SHUT_WR);
  } while (return_code == GNUTLS_E_AGAIN || return_code == GNUTLS_E_INTERRUPTED);

  gnutls_deinit(session);
}

