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

    if (!SSL_CTX_load_verify_locations(connectorinfo->ctx, connectorinfo->ca_cert_file, NULL)) {
        fprintf(stderr,"Error setting ca certificate\n");
		ERR_print_errors_fp(stderr);
		exit(ERR_EXIT);
    }


    if (!SSL_CTX_set_min_proto_version(connectorinfo->ctx, connectorinfo->tls_minimum_version)) {
        fprintf(stderr,"Error setting minimum TLS version in SSL Context!!\n");
		ERR_print_errors_fp(stderr);
		exit(ERR_EXIT);
	}

	if (!SSL_CTX_set_max_proto_version(connectorinfo->ctx, connectorinfo->tls_maximum_version )) {
        fprintf(stderr,"Error setting maximum TLS version in SSL Context!!\n");
		ERR_print_errors_fp(stderr);
		exit(ERR_EXIT);
	}

	if (connectorinfo->tls_verify_client != VERIFY_UNDEV) {
		switch (connectorinfo->tls_verify_client) {
			case VERIFY_REQUIRE:
				SSL_CTX_set_verify(connectorinfo->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE, NULL);
				break;
			case VERIFY_OPTIONAL:
				SSL_CTX_set_verify(connectorinfo->ctx, SSL_VERIFY_PEER, NULL);
				break;
			case VERIFY_NONE: 
			default:
				connectorinfo->tls_verify_client = VERIFY_NONE;
				SSL_CTX_set_verify(connectorinfo->ctx, SSL_VERIFY_NONE, NULL);
				break;
		}
		SSL_CTX_set_verify_depth(connectorinfo->ctx, connectorinfo->tls_verify_client_depth);
	}

	if (connectorinfo->cipher_suites != NULL) {
		if (!SSL_CTX_set_ciphersuites(connectorinfo->ctx,connectorinfo->cipher_suites)) {
			fprintf(stderr,"Error setting TLS1.3 ciphers suites \"%s\" in SSL Context!!\n",connectorinfo->cipher_suites);
			ERR_print_errors_fp(stderr);
			exit(ERR_EXIT);
		}
	}

	if (connectorinfo->ciphers != NULL) {
		if (!SSL_CTX_set_cipher_list(connectorinfo->ctx,connectorinfo->ciphers)) {
			fprintf(stderr,"Error setting ciphers \"%s\" in SSL Context!!\n",connectorinfo->ciphers);
			ERR_print_errors_fp(stderr);
			exit(ERR_EXIT);
		}
	}

	if (connectorinfo->tls_crl == CRL_LEAF || connectorinfo->tls_crl == CRL_CHAIN) {
		X509_VERIFY_PARAM *param = X509_VERIFY_PARAM_new();

		switch (connectorinfo->tls_crl) {
			case CRL_LEAF:
				X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
				break;
			case CRL_CHAIN:
				X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL| X509_V_FLAG_TRUSTED_FIRST); 
				break;
		}

		SSL_CTX_set1_param(connectorinfo->ctx,param);
		X509_VERIFY_PARAM_free(param);

		if (connectorinfo->tls_crl_file == NULL) {
			DO_SYSL("Config-File: CRL check activated but no CRL location defined! All checks will fail!");
		}

		X509_STORE *cert_store = SSL_CTX_get_cert_store(connectorinfo->ctx);
		if (cert_store != NULL) {
			fprintf(stderr, connectorinfo->tls_crl_file);
			if (!X509_STORE_load_locations(cert_store,connectorinfo->tls_crl_file,NULL)) {
        		fprintf(stderr,"Error setting CRL location in SSL Context!\n");
				ERR_print_errors_fp(stderr);
				exit(ERR_EXIT);
			}
		} else {
			fprintf(stderr,"X509_STORE could not be loaded from SSL Context!\n");
		}
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
check_tls_prerequisites(connectorinfo_t *connectorinfo)
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
tls_session_init(server_cb_inf *inf)
{
	char *connection_log = NULL;
	inf->sockinf->tls_session = SSL_new(inf->sockinf->connectorinfo->ctx);

	if(!SSL_set_fd(inf->sockinf->tls_session,inf->sockinf->sockfd)) {
		fprintf(stderr,"Error creating TLS session!!\n");
		ERR_print_errors_fp(stderr);
		return FALSE;
	} 

	if (SSL_accept(inf->sockinf->tls_session) <=0) {
		fprintf(stderr,"Error negotiating TLS session!!\n");
		ERR_print_errors_fp(stderr);
		return FALSE;
	}

	/* Log the started connection */
	char conn_s[50];
	sprintf(conn_s,"%s:%d",inf->sockinf->connectorinfo->listen,inf->sockinf->connectorinfo->port);
	connection_log = str_concat("Created TLS connection from ", inf->sockinf->ip, " Connector:", conn_s, NULL);
	DO_SYSL(connection_log);
	fprintf(stderr,"%s\n",connection_log);
	FFLUSH
	free(connection_log);

	return TRUE;
}

void
tls_session_close(SSL *session)
{
	SSL_shutdown(session);
	SSL_free(session);

	DO_SYSL("TLS session shutdown")
}