/* $FreeBSD: src/kerberos5/include/crypto-headers.h,v 1.3 2008/05/07 13:53:03 dfr Exp $ */
#ifndef __crypto_headers_h__
#define __crypto_headers_h__
#define OPENSSL_DES_LIBDES_COMPATIBILITY
#include <openssl/evp.h>
#include <openssl/des.h>
#include <openssl/rc4.h>
#include <openssl/md2.h>
#include <openssl/md4.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/ui.h>
#include <openssl/rand.h>
#include <openssl/engine.h>
#include <openssl/pkcs12.h>
#include <openssl/hmac.h>
#endif /* __crypto_headers_h__ */
