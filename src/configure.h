/*
MIT License

Copyright (c) 2020 kmwebnet

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef CONFIGURE_H
#define CONFIGURE_H


#include "stdio.h"
#include "stdlib.h"

#include "cryptoauthlib.h"
#include "atcacert/atcacert_client.h"
#include "atcacert/atcacert_pem.h"
#include "crypto/atca_crypto_sw_sha1.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "cert_chain.h"

typedef enum {
    offlineboot,
	onlineboot,
	forcefwdownload,
	bootprohibited
} statetype;

/*
typedef enum {
	noneresponse,
	otastart,
	getnextkey,
	certificate,
	seturl
} actiontype;
*/

extern char *statestr[];

ATCAIfaceCfg cfg;
void get_atecc608cfg(ATCAIfaceCfg *cfg);


statetype getcurrentstate(void);
void setcurrentstate(statetype state);

void savemiscdata (uint8_t * data);
void restoremiscdata (uint8_t * data);


/**
 * @brief      get public key from ATECC608A
 *             
 * @param[out] nextkey  output next candidate key slot no.
 *
 * @param[out] pem  PEM formatted public key
 *
 * @return
 *     - ATCA_STATUS
 */
ATCA_STATUS getpubkey(uint8_t * nextkey, char *pem);

char *replace_chr(char *str, const char what, const char *with);

char *replace_str_all(char *str, const char *what, const char *with);

ATCA_STATUS pickpemcert(char *buffer , char *pem);

ATCA_STATUS setdevicecert(char *devicecertpem , uint8_t nextkey);

uint8_t sysinit(void);

#endif