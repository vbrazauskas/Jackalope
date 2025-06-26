/*
Copyright 2020 Google LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include <cstddef> // for std::size_t

// ensure we can find the target
#define FUZZ_TARGET_MODIFIERS __declspec(dllexport)

typedef wchar_t         WChar;
typedef wchar_t*        WCharP;
typedef unsigned char   BYTE;

int Base64Encode(const BYTE* inputBuffer, uint32_t inputLen, WCharP outputBuffer, uint32_t outputLen)
{
    static WChar b64table[65] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    int mod = inputLen % 3;
    uint32_t reqOutput = (inputLen / 3) * 4 + (3 - mod) % 3 + 1;
    if (outputLen < reqOutput)
    {
        //GetLogger()->errorv (L"Data buffer too small in b64 %u < %u", outputLen, reqOutput);
        return 0;
    }

    WCharP pOutput = outputBuffer;
    uint32_t iChar = 0;
    while (iChar < inputLen - mod)
    {
        *pOutput++ = b64table[inputBuffer[iChar++] >> 2];
        *pOutput++ = b64table[((inputBuffer[iChar - 1] << 4) | (inputBuffer[iChar] >> 4)) & 0x3f];
        *pOutput++ = b64table[((inputBuffer[iChar] << 2) | (inputBuffer[iChar + 1] >> 6)) & 0x3f];
        *pOutput++ = b64table[inputBuffer[iChar + 1] & 0x3f];
        iChar += 2;
    }

    if (0 != mod)
    {
        *pOutput++ = b64table[inputBuffer[iChar++] >> 2];
        *pOutput++ = b64table[((inputBuffer[iChar - 1] << 4) | (inputBuffer[iChar] >> 4)) & 0x3f];

        if (1 == mod)
        {
            *pOutput++ = L'=';
            *pOutput++ = L'=';
        }
        else
        {
            *pOutput++ = b64table[(inputBuffer[iChar] << 2) & 0x3f];
            *pOutput++ = L'=';
        }
    }
    *pOutput++ = L'\0';
    return 1;
}

std::size_t calculateBase64Length(std::size_t inputLength) {
    return ((inputLength + 2) / 3) * 4;
}

// actual target function

void FUZZ_TARGET_MODIFIERS fuzz(char *name) {
  unsigned char *sample_bytes = NULL;
  uint32_t sample_size = 0;
  
  // read the sample either from file
  FILE *fp = fopen(name, "rb");
  if(!fp) {
    printf("Error opening %s\n", name);
    return;
  }
  fseek(fp, 0, SEEK_END);
  sample_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  sample_bytes = (unsigned char *)malloc(sample_size);
  fread(sample_bytes, 1, sample_size, fp);
  fclose(fp);

  std::size_t inputLength = sample_size;
  std::size_t base64Length = calculateBase64Length(inputLength) + 1;

  WCharP output = new wchar_t[base64Length];

  unsigned int status = Base64Encode(sample_bytes, sample_size, output, base64Length);

  if(output) delete[] output;

  if(sample_bytes) free(sample_bytes);
}

int main(int argc, char **argv)
{
  if(argc != 3) {
    printf("Usage: %s <-f> <file name>\n", argv[0]);
    return 0;
  }
  
  fuzz(argv[2]);
  
  return 0;
}
