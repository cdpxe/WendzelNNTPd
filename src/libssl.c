/*
 * WendzelNNTPd is distributed under the following license:
 *
 * Copyright (c) 2004-2024 Steffen Wendzel <wendzel (at) hs-worms (dot) de>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.	If not, see <http://www.gnu.org/licenses/>.
 */

#include "main.h"

extern char *tls_cipher_prio; /* config.y */

static SSL_CTX *context;

void
tls_global_init(connectorinfo_t *connectorinfo)
{
    connectorinfo->ctx=SSL_CTX_new(TLS_server_method());
					
	if (!connectorinfo->ctx) {
        fprintf(stderr,"Error creating SSL Context!\n");
		ERR_print_errors_fp(stderr);
		exit(ERR_EXIT);
	}

    if (!SSL_CTX_use_PrivateKey_file(connectorinfo->ctx,connectorinfo->server_key_file,SSL_FILETYPE_PEM)) {
        fprintf(stderr,"Error loading private key file \"%s\" in SSL Context!!\n",connectorinfo->server_key_file);
		ERR_print_errors_fp(stderr);
		exit(ERR_EXIT);
	}

	if (!SSL_CTX_use_certificate_file(connectorinfo->ctx,connectorinfo->server_cert_file,SSL_FILETYPE_PEM)) {
        fprintf(stderr,"Error loading certificate \"%s\" in SSL Context!!\n",connectorinfo->server_cert_file);
		ERR_print_errors_fp(stderr);
		exit(ERR_EXIT);
	}

	if (!SSL_CTX_check_private_key(connectorinfo->ctx)) {
        fprintf(stderr,"Private key in \"%s\" does not match Certificate in \"%s\" !\n",connectorinfo->server_key_file,connectorinfo->server_cert_file);
		ERR_print_errors_fp(stderr);
		exit(ERR_EXIT);
	}

	SSL_CTX_set_verify(connectorinfo->ctx, SSL_VERIFY_NONE, NULL);

    if (!SSL_CTX_load_verify_locations(connectorinfo->ctx, connectorinfo->ca_cert_file, NULL)) {
        fprintf(stderr,"Error setting ca certificate\n");
		ERR_print_errors_fp(stderr);
		exit(ERR_EXIT);
    }


    if (!SSL_CTX_set_min_proto_version(connectorinfo->ctx, TLS1_VERSION)) {
        fprintf(stderr,"Error setting min TLS version in SSL Context!!\n");
		ERR_print_errors_fp(stderr);
		exit(ERR_EXIT);
	}

	if (!SSL_CTX_set_max_proto_version(connectorinfo->ctx, TLS1_3_VERSION )) {
        fprintf(stderr,"Error setting max TLS version in SSL Context!!\n");
		ERR_print_errors_fp(stderr);
		exit(ERR_EXIT);
	}

    if (!SSL_CTX_set_ciphersuites(connectorinfo->ctx,"TLS_AES_256_GCM_SHA384")) {
        fprintf(stderr,"Error setting TLS1.3 ciphers suites \"%s\" in SSL Context!!\n",connectorinfo->cipher_suites);
		ERR_print_errors_fp(stderr);
		exit(ERR_EXIT);
	}

	if (!SSL_CTX_set_cipher_list(connectorinfo->ctx,"ALL")) {
        fprintf(stderr,"Error setting ciphers \"%s\" in SSL Context!!\n",connectorinfo->ciphers);
		ERR_print_errors_fp(stderr);
		exit(ERR_EXIT);
	}

	fprintf(stdout, "TLS initialized\n");
}

void
initialize_connector_ports(connectorinfo_t *connectorinfo)
{
    if (connectorinfo->enable_tls) {
        connectorinfo->port = DEFAULT_TLS_PORT; 
    } else {
        connectorinfo->port = DEFAULT_PORT;
    }
}

int
check_ssl_prerequisites(connectorinfo_t *connectorinfo)
{
    if (connectorinfo->server_cert_file == NULL) {
        fprintf(stderr,"There was no certificate file defined!\n");
        ERR_print_errors_fp(stderr);
		return FALSE;
    }

    if (connectorinfo->server_key_file == NULL) {
        fprintf(stderr,"There was no server key file defined!\n");
        ERR_print_errors_fp(stderr);
		return FALSE;
    }

    if (access(connectorinfo->server_cert_file,R_OK) != 0) {
        fprintf(stderr,"Certificate file is not found in defined path!\n");
        ERR_print_errors_fp(stderr);
		return FALSE;
    }

    if (access(connectorinfo->server_key_file,R_OK) != 0) {
        fprintf(stderr,"Server key file is not found in defined path!\n");
        ERR_print_errors_fp(stderr);
		return FALSE;
    }

    return TRUE;
}

void
tls_global_close()
{
	SSL_CTX_free(context);

	DO_SYSL("TLS shutdown");
}

int
tls_session_init(SSL **session, int sockfd)
{
	return TRUE;
}

void
tls_session_close(SSL *session)
{
	SSL_shutdown(session);
	SSL_free(session);

	DO_SYSL("TLS session shutdown")
}