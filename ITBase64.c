/*
 *  ITBase64.c
 *  Base64Encoder
 *
 *  Copyright 2010 Stefan Reinhold (development@ithron.de). All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification, are
 *  permitted provided that the following conditions are met:
 * 
 *  1. Redistributions of source code must retain the above copyright notice, this list of
 *  conditions and the following disclaimer.
 * 
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list
 *  of conditions and the following disclaimer in the documentation and/or other materials
 *  provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY STEFAN REINHOLD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 *  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STEFAN REINHOLD OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 *  The views and conclusions contained in the software and documentation are those of the
 *  authors and should not be interpreted as representing official policies, either expressed
 *  or implied, of Stefan Reinhold.
 */

#include "ITBase64.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define isWhiteSpace(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')

static const char ITBase64AdditionsEncodingTable[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* function declarations */
void encodeWord(const uint8_t *bytes, char *dest, const int lastWord);
unsigned int decodeWord(const char *chars, uint8_t *dest);
uint8_t charToByte(const char c);
unsigned int whiteSpaceFreeStringCreate(char **dest, const char *str, unsigned int len);

/*! \brief Base64 encodes the 3 given bytes into 4 characters.
 *
 *  \param[in] bytes A pointer to a memory location that contains at least 3 bytes.
 *  \param[out] dest A pointer to a memory location that must have enaugh space to hold 4 bytes.
 *                   The encoded characters are put there.
 *  \param[in] lastWord A flag that indicates if the function is about to decode the last words
 *                      of data. So it fills it up with zeros it the flag is set.
 */
inline void encodeWord(const uint8_t *bytes, char *dest, const int lastWord) {
	
	// convert bytes to characters by using the encoding table
	dest[0] = ITBase64AdditionsEncodingTable[(bytes[0] & 0xFC) >> 2];
	dest[1] = ITBase64AdditionsEncodingTable[((bytes[0] & 0x03) << 4) | ((bytes[1] & 0xF0) >> 4)];
	dest[2] = ITBase64AdditionsEncodingTable[((bytes[1] & 0x0F) << 2) | ((bytes[2] & 0xC0) >> 6)];
	
	// replace 'zero' bytes with '='
	if (dest[2] == 'A' && lastWord) {
		
		memset(&dest[2], '=', 2);
		return;
	}
	
	dest[3] = ITBase64AdditionsEncodingTable[bytes[2] & 0x3F];
	
	if (dest[3] == 'A' && lastWord) {
		
		dest[3] = '=';
	}
}

/*! \brief Converts the given base64 character to a (6 bit) byte
 *         Returns 0xff on error.
 */
inline uint8_t charToByte(const char c) {
	
	if (c >= 'A' && c <= 'Z')
		return c - 'A';
	else if (c >= 'a' && c <= 'z')
		return c - 'a' + 26;
	else if (c >= '0' && c <= '9')
		return c - '0' + 52;
	else if (c == '+')
		return 62;
	else if (c == '/')
		return 63;
	else if (c == '=')
		return 0;
	else //error
		return 0xff;
}

/*! Decodes the given 4 base64 characters to less than or equal to 3 bytes of data.
 *  The number of bytes encoded is retured. 0 on error.
 */
inline unsigned int decodeWord(const char *chars, uint8_t *dest) {
	
	const uint8_t c0 = charToByte(chars[0]);
	const uint8_t c1 = charToByte(chars[1]);
	
	if (c0 == 0xff || c1 == 0xff)
		return 0;
	
	dest[0] = (c0 << 2) | ((c1 & 0x30) >> 4);
	
	if (chars[2] == '=')
		return 1;
	
	const uint8_t c2 = charToByte(chars[2]);
	
	if (c2 == 0xff)
		return 0;
	
	dest[1] = ((c1 & 0x0f) << 4) | ((c2 & 0x3c) >> 2);
	
	if (chars[3] == '=')
		return 2;
	
	const uint8_t c3 = charToByte(chars[3]);
	
	if (c3 == 0xff)
		return 0;
	
	dest[2] = ((c2 & 0x03) << 6) | c3;
	
	return 3;
}

/*! Removes all whitespaces from the given string. Note that a new (newly allocated) string is returned,
 *  so remember to free it when you used it.
 */
inline unsigned int whiteSpaceFreeStringCreate(char **dest, const char *str, unsigned int len) {
	
	unsigned int i = 0, resultLen = 0;
	*dest = (char *) malloc(len * sizeof(char));
	char *destPtr = *dest;
	char c;
	
	for (i = 0; i < len; i++) {
		c = str[i];
		if (!isWhiteSpace(c)) 
			destPtr[resultLen++] = c;
	}
	
	return resultLen;
}

char * ITBase64EncodedStringCreate(const void *data, const unsigned int len, unsigned int *stringLength) {
	
	const unsigned int resultSize = ((len / 3) + ((len % 3) != 0)) * 4;
	const uint8_t *ptr = (uint8_t *)data;
	const unsigned int restLen = len % 3;
	const unsigned int effLen = len - restLen;
	unsigned int srcIndex = 0, destIndex = 0;
	
	char *dest = (char *) malloc((resultSize + 1) * sizeof(char));
	
	while (srcIndex < effLen) {
		
		encodeWord(&ptr[srcIndex], &dest[destIndex], 0);
		
		srcIndex += 3;
		destIndex += 4;
	}
	
	if (restLen != 0) {
		
		uint8_t restWord[3] = {
			0, 0, 0
		};
		
		memcpy(restWord, &ptr[effLen], restLen);
		
		encodeWord(restWord, &dest[resultSize - 4], 1);
	}
	
	dest[resultSize] = '\0';
	
	if (stringLength != NULL)
		*stringLength = resultSize;
	
	return dest;
}

void * ITBase64DecodedDataCreate(const char *string, const unsigned int len, unsigned int *dataLength) {
	
	unsigned int srcIndex = 0, destIndex = 0, bytesDecoded = 0;
	char *characters = NULL;
	const unsigned int effLen = whiteSpaceFreeStringCreate(&characters, string, len);
	const unsigned int maxLen = 3 * effLen / 4;
	
	if (effLen % 4 != 0) {
		*dataLength = 0;
		free(characters);
		return NULL;
	}
	
	uint8_t *dest = (uint8_t *) malloc(maxLen);
	
	while (srcIndex < effLen) {
		
		bytesDecoded = decodeWord(&characters[srcIndex], &dest[destIndex]);
		
		if (bytesDecoded < 3)
			break;
		
		srcIndex += 4;
		destIndex += 3;
	}
	
	if (bytesDecoded > 0 && bytesDecoded < 3) {
		
		*dataLength = destIndex + bytesDecoded;
		
		dest = (uint8_t *) realloc(dest, *dataLength);
	}
	else if (bytesDecoded == 0) {
		
		// error
		free(dest);
		dest = NULL;
		*dataLength = 0;
	}
	else {
		
		*dataLength = destIndex;
	}
	
	free(characters);
	
	return dest;		
}
