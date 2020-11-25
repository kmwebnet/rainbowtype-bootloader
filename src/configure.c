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
#include "configure.h"
#include "esp_tls.h"
#include "cert_chain.h"

char *statestr[] = {
    "offlineboot",
	"onlineboot",
	"forcefwdownload",
	"bootprohobited"
};

const uint8_t public_key_x509_header[] = {
    0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A,
    0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04
};

static uint8_t secdata[32] = {};

extern uint8_t iokeyrandom[32];

static void savedevstatus (uint8_t * data)
{

    get_atecc608cfg(&cfg);
    ATCA_STATUS status = atcab_init(&cfg);

    if (status != ATCA_SUCCESS) {
        printf("atcab_init() failed with ret=0x%08d\r\n", status);
    }

    atca_mbedtls_ecdh_ioprot_cb(iokeyrandom);
    uint8_t num_in[NONCE_NUMIN_SIZE] = { 0 };

    if (ATCA_SUCCESS != atcab_write_enc(8 , 4 , data ,iokeyrandom ,  6 , num_in)) {
   	printf("read data to slot8 failed");
    }

return;
}

static void restoredevstatus (uint8_t * data)
{

    get_atecc608cfg(&cfg);
    ATCA_STATUS status = atcab_init(&cfg);

    if (status != ATCA_SUCCESS) {
        printf("atcab_init() failed with ret=0x%08d\r\n", status);
    }

    atca_mbedtls_ecdh_ioprot_cb(iokeyrandom);
    uint8_t num_in[NONCE_NUMIN_SIZE] = { 0 };

    if (ATCA_SUCCESS != atcab_read_enc(8 , 4 , data ,iokeyrandom ,  6 , num_in)) {
   	printf("read data to slot8 failed");
    }

return;
}

statetype getcurrentstate(void)
{
    restoredevstatus(secdata);
    return secdata[3]; 

}

void setcurrentstate(statetype state)
{
    secdata[3] = state; 
    savedevstatus(secdata);
}



void savemiscdata (uint8_t * data)
{

    get_atecc608cfg(&cfg);
    ATCA_STATUS status = atcab_init(&cfg);

    if (status != ATCA_SUCCESS) {
        printf("atcab_init() failed with ret=0x%08d\r\n", status);
    }

    atca_mbedtls_ecdh_ioprot_cb(iokeyrandom);
    uint8_t num_in[NONCE_NUMIN_SIZE] = { 0 };

    for (int i = 0 ;  i < 8 ; i++)
    {
        if (ATCA_SUCCESS != atcab_write_enc(8 , 5 + i , &data[32 * i] ,iokeyrandom ,  6 , num_in)) 
        {
   	    printf("read data to slot8 failed");
        }
    }


return;
}

void restoremiscdata (uint8_t * data)
{

    get_atecc608cfg(&cfg);
    ATCA_STATUS status = atcab_init(&cfg);

    if (status != ATCA_SUCCESS) {
        printf("atcab_init() failed with ret=0x%08d\r\n", status);
    }

    atca_mbedtls_ecdh_ioprot_cb(iokeyrandom);
    uint8_t num_in[NONCE_NUMIN_SIZE] = { 0 };

    for (int i = 0 ;  i < 8 ; i++)
    {
        if (ATCA_SUCCESS != atcab_read_enc(8 , 5 + i , &data[32 * i] ,iokeyrandom ,  6 , num_in)) 
        {
   	    printf("read data to slot8 failed");
        }
    }

return;
}



ATCA_STATUS getpubkey(uint8_t * nextkey, char *pem)
{
    ATCA_STATUS status;

    uint8_t buf[128];
    uint8_t * tmp;
    size_t buf_len = sizeof(buf);
    uint8_t pubkey[ATCA_PUB_KEY_SIZE];

    restoredevstatus(secdata);

    //get current key no. and determine next key no.
    if (secdata[0] == 0)
    {
        *nextkey = 1;
        secdata[1] = 1;
    } 
    else if (secdata[0] == 1)
    {
        *nextkey = 0;
        secdata[1] = 0;
    } 
    else
    {
        return ATCA_BAD_PARAM;
    }
    

    get_atecc608cfg(&cfg);
    if (ATCA_SUCCESS != (status = atcab_init(&cfg)))
    {
        printf("Unable to initialize interface: %x\r\n", status);
        goto exit;
    }

    /* Generate new keys */
    if (ATCA_SUCCESS != (status = atcab_get_pubkey(*nextkey, pubkey)))
    {
        printf("Get Pubkey on slot %d failed: %x\r\n", *nextkey, status);
        goto exit;
    }
    else
    {
    /* Calculate where the raw data will fit into the buffer */
    tmp = buf + sizeof(buf) - ATCA_PUB_KEY_SIZE - sizeof(public_key_x509_header);

    /* Copy the header */
    memcpy(tmp, public_key_x509_header, sizeof(public_key_x509_header));

    /* Copy the key bytes */
    memcpy(tmp + sizeof(public_key_x509_header), pubkey, ATCA_PUB_KEY_SIZE);

    /* Convert to base 64 */
    (void)atcab_base64encode(tmp, ATCA_PUB_KEY_SIZE + sizeof(public_key_x509_header), (char *)buf, &buf_len);

    /* Add a null terminator */
    buf[buf_len] = 0;

    /* Print out the key */
    sprintf(pem, "-----BEGIN PUBLIC KEY-----\n%s\n-----END PUBLIC KEY-----\n", buf);

    //set cert request flag
    secdata[2] = 1 ;

    savedevstatus(secdata);

    }

exit:
    return status;
}

char *replace_chr(char *str, const char what, const char *with)
{
  if (!what) { return str; }
  char *what_pos = str;
  const size_t with_len = strlen(with);
  while ((what_pos = strchr(what_pos, what)) != NULL) {
    const char *remain = what_pos + 1;
    memmove(what_pos + with_len, remain, strlen(remain) + 1);
    memcpy(what_pos, with, with_len);
  }
  return str;
}

char *replace_str_all(char *str, const char *what, const char *with)
{
  char *what_pos = str;
  const size_t what_len = strlen(what);
  const size_t with_len = strlen(with);
  while ((what_pos = strstr(what_pos, what)) != NULL) { 
    const char *remain = what_pos + what_len; 
    memmove(what_pos + with_len, remain, strlen(remain) + what_len);
    memcpy(what_pos, with, with_len);
  }
  return str;
}


//pick up cert pem from buffer

ATCA_STATUS pickpemcert(char *buffer , char *pem)
{

char * pemstart = strstr(buffer , PEM_CERT_BEGIN);
    if (pemstart == NULL)
    {
        return ATCA_BAD_PARAM;
    }

char * pemend = strstr(buffer , PEM_CERT_END);
    if (pemend == NULL)
    {
        return ATCA_BAD_PARAM;
    }

//calculate pem length
int length;
length = pemend - pemstart + 25;

// check malloc
    if (length > 1000)
    {
        return ATCA_BAD_PARAM;
    }

memcpy(pem, pemstart, length);

return ATCA_SUCCESS;
}

/* Writes the certificates into a device*/
ATCA_STATUS setdevicecert(char *devicecertpem , uint8_t nextkey)
{

	ATCA_STATUS status;

    restoredevstatus(secdata);

    //check if device state is cert accept mode
    if (secdata[2] != 1)
    {
        status = ATCA_EXECUTION_ERROR;
        goto exit;
    }

    //check current key no. and next key no. are not the same
    if (secdata[0] == nextkey)
    {
        status = ATCA_BAD_PARAM;
        goto exit;
    }

    //check registered next key no. and ordered one are the same
    if (secdata[1] != nextkey)
    {
        status = ATCA_BAD_PARAM;
        goto exit;
    }


    uint8_t ca_key[64];


	int i;
	bool diff = false;

    uint8_t backupdevicecertder[1000];
    size_t backupcertlen = 1000;
    uint8_t next_device_cert[1000];
    uint8_t verifycert[1000];
    size_t derbuffer;

    get_atecc608cfg(&cfg);
    if (ATCA_SUCCESS != (status = atcab_init(&cfg)))
    {
        printf("Unable to initialize interface: %x\r\n", status);
        goto exit;
    }

    atcacert_def_t  nextdevicecertdef;
    nextdevicecertdef = g_cert_def_2_device;
    atcacert_def_t * devicecertdefp;
    devicecertdefp = &nextdevicecertdef;
    atcacert_device_loc_t * ca_key_cfg = &devicecertdefp->ca_cert_def->public_key_dev_loc;
    status = atcab_read_pubkey(ca_key_cfg->slot, ca_key);

	/* back up the device certificate */
	printf("Backing Up Device Certificate\r\n");
	if (ATCA_SUCCESS != (status = atcacert_read_cert(&g_cert_def_2_device,
		(const uint8_t *)ca_key, backupdevicecertder, &backupcertlen)))
	{
		printf("Failed to read device certificate: %d\r\n", status);
        goto exit;
	}

	/* convert new device certificate */
    derbuffer = 1000;
    if (ATCA_SUCCESS != (status = atcacert_decode_pem(devicecertpem, strlen(devicecertpem) , next_device_cert , &derbuffer , PEM_CERT_BEGIN, PEM_CERT_END ))) {
	printf("device cert conversion failed:%d\n", status);
    goto exit;
    }

    nextdevicecertdef.public_key_dev_loc.slot = (uint16_t)nextkey ;

	/* Write new device certificate */
	printf("Writing Device Certificate\r\n");
	if (ATCA_SUCCESS != (status = atcacert_write_cert(&nextdevicecertdef, next_device_cert, derbuffer)))
	{
		printf("Failed to write device certificate: %d\n", status);
		goto exit;
	}

	/* Read back the device certificate */
	size_t tmp_size = sizeof(verifycert); //device_size + 10;
	printf("Reading Device Certificate\r\n");
	if (ATCA_SUCCESS != (status = atcacert_read_cert(&nextdevicecertdef,
		(const uint8_t *)ca_key, verifycert, &tmp_size)))
	{
		printf("Failed to read back device certificate: %d\r\n", status);
        goto exit;
	}

	/* Compare the device certificate */
	printf("Comparing Device Certificate\r\n");
	if (memcmp(next_device_cert, verifycert, tmp_size))
	{
		printf("Device certificate missmatch\r\n");

		diff = false;
		for (i = 0; i < tmp_size; i++)
		{
			if (next_device_cert[i] != verifycert[i])
			{
				diff = true;
			}

			if (0 == (i % 16))
			{
				printf("%s\r\n%04X: ", diff ? "*" : "", i);
				diff = false;
			}
			printf("%02X|%02X ", next_device_cert[i], verifycert[i]);
		}

		printf("Trying to recover previous cert\r\n");

// restore certificate

    	if (ATCA_SUCCESS != (status = atcacert_write_cert(&g_cert_def_2_device,
    		backupdevicecertder, backupcertlen)))
    	{
    		printf("Failed to write previous device certificate: %d\r\n", status);
            goto exit;
    	}

/* Read back the device certificate */
        memset(verifycert, 0, 1000);
    	tmp_size = 1000; //device_size + 10;
	    printf("Reading Device Certificate\r\n");
	    if (ATCA_SUCCESS != (status = atcacert_read_cert(&g_cert_def_2_device,
	    	(const uint8_t *)ca_key, verifycert, &tmp_size)))
	    {
	    	printf("Failed to read back previous device certificate: %d\r\n", status);
            goto exit;
	    }        

    	/* Compare the device certificate */
	    printf("Comparing Device Certificate\r\n");
	    if (memcmp(backupdevicecertder, verifycert, tmp_size))
	    {
		    printf("Device certificate missmatch\r\n");

		    diff = false;
		    for (i = 0; i < tmp_size; i++)
		    {
			    if (backupdevicecertder[i] != verifycert[i])
			    {
			    	diff = true;
			    }

			    if (0 == (i % 16))
			    {
				    printf("%s\r\n%04X: ", diff ? "*" : "", i);
				    diff = false;
			    }
			    printf("%02X|%02X ", backupdevicecertder[i], verifycert[i]);
		    }
        }
		printf("failed to recover previous certificate. unknown error.\r\n");
        status = ATCA_EXECUTION_ERROR; 
        goto exit;

	}

    /* Generate new keys */
    if (ATCA_SUCCESS != (status = atcab_genkey_base(GENKEY_MODE_PRIVATE, g_cert_def_2_device.public_key_dev_loc.slot, NULL, NULL)))
    {
        printf("Genkey on next slot failed: %x\r\n", status);
    }
    else
    {
	printf("\r\nDevice certificate rolling Successful!\r\n");
    status = ATCA_SUCCESS;

    //set current key no. as ordered
    secdata[0] = nextkey;
    g_cert_def_2_device.public_key_dev_loc.slot = (uint16_t)nextkey;

    //reset cert accept mode
    secdata[2] = 0;

    savedevstatus(secdata);

    }
	/* End the session */

exit:
	return status;
}


uint8_t sysinit(void)
{

    restoredevstatus(secdata);

    printf("current key slot is :%d\n\n", secdata[0]);
    
    g_cert_def_2_device.public_key_dev_loc.slot = secdata[0];

    return secdata[0] ;

}

	
