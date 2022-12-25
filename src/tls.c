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
extern char *tls_ca_file; /* config.y */
extern char *tls_cert_file; /* config.y */
extern char *tls_key_file; /* config.y */
extern char *tls_crl_file; /* config.y */
extern char *tls_cipher_prio; /* config.y */

static gnutls_certificate_credentials_t x509_credentials;
static gnutls_priority_t tls_cipher_priorities;

#define CHECK(cmd)  \
  return_code = cmd; \
  if (return_code != GNUTLS_E_SUCCESS) { \
    DO_SYSL("TLS not initialized, error in") \
    DO_SYSL(#cmd) \
    fprintf(stderr, "TLS not initialized, %s error\n", #cmd); \
    fprintf(stderr, "%s\n", gnutls_strerror(return_code)); \
    return FALSE; \
  }

#define CHECK_LT_ZERO(cmd)  \
  return_code = cmd; \
  if (return_code < 0) { \
    DO_SYSL("TLS not initialized, error in") \
    DO_SYSL(#cmd) \
    fprintf(stderr, "TLS not initialized, %s error\n", #cmd); \
    fprintf(stderr, "%s\n", gnutls_strerror(return_code)); \
    return FALSE; \
  }

int
tls_global_init()
{
  int return_code;

  CHECK(gnutls_global_init())
  CHECK(gnutls_certificate_allocate_credentials(&x509_credentials))

  /* returns the number of processed certs, or an error */
  CHECK_LT_ZERO(gnutls_certificate_set_x509_trust_file(x509_credentials, tls_ca_file, GNUTLS_X509_FMT_PEM))

  if (tls_crl_file) {
    CHECK(gnutls_certificate_set_x509_crl_file(x509_credentials, tls_crl_file, GNUTLS_X509_FMT_PEM))
  }

  CHECK(gnutls_certificate_set_x509_key_file(x509_credentials, tls_cert_file, tls_key_file, GNUTLS_X509_FMT_PEM))

#if GNUTLS_VERSION_NUMBER >= 0x030506
  /* only available since GnuTLS 3.5.6 */
  CHECK(gnutls_certificate_set_known_dh_params(x509_credentials, GNUTLS_SEC_PARAM_MEDIUM))
#endif

  const char *err;
  return_code = gnutls_priority_init(&tls_cipher_priorities, tls_cipher_prio, &err);
  if (return_code != GNUTLS_E_SUCCESS) {
    DO_SYSL("TLS not initialized, could not set cipher priorities");

    fprintf(stderr, "TLS not initialized, could not set cipher prioritires (at %s)\n", err);
    fprintf(stderr, "%s\n", gnutls_strerror(return_code));
    return FALSE;
  }

  DO_SYSL("TLS initialized");
  return TRUE;
}

void
tls_global_close()
{
    gnutls_certificate_free_credentials(x509_credentials);
    gnutls_priority_deinit(tls_cipher_priorities);
    gnutls_global_deinit();

    DO_SYSL("TLS shutdown")
}

int
tls_session_init(gnutls_session_t *session, int sockfd)
{
  int return_code;

  CHECK(gnutls_init(session, GNUTLS_SERVER))
  CHECK(gnutls_priority_set(*session, tls_cipher_priorities))
  CHECK(gnutls_credentials_set(*session, GNUTLS_CRD_CERTIFICATE, x509_credentials))

  if (tls_mutual_auth) {
    gnutls_certificate_server_set_request(*session, GNUTLS_CERT_REQUIRE);
  } else {
    gnutls_certificate_server_set_request(*session, GNUTLS_CERT_IGNORE);
  }

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

  DO_SYSL("TLS session init")
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
  DO_SYSL("TLS session shutdown")
}
