#include "SHA_AES.h"

#include "versat_accel.h"

#include <string.h>

#include "unitConfiguration.h"

#include "iob-uart16550.h"
#include "printf.h"

#undef  ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define nullptr 0

const uint8_t sbox[256] = {
   0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
   0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
   0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
   0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
   0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
   0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
   0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
   0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
   0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
   0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
   0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
   0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
   0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
   0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
   0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
   0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };

const uint8_t mul2[] = {
   0x00,0x02,0x04,0x06,0x08,0x0a,0x0c,0x0e,0x10,0x12,0x14,0x16,0x18,0x1a,0x1c,0x1e,
   0x20,0x22,0x24,0x26,0x28,0x2a,0x2c,0x2e,0x30,0x32,0x34,0x36,0x38,0x3a,0x3c,0x3e,
   0x40,0x42,0x44,0x46,0x48,0x4a,0x4c,0x4e,0x50,0x52,0x54,0x56,0x58,0x5a,0x5c,0x5e,
   0x60,0x62,0x64,0x66,0x68,0x6a,0x6c,0x6e,0x70,0x72,0x74,0x76,0x78,0x7a,0x7c,0x7e,
   0x80,0x82,0x84,0x86,0x88,0x8a,0x8c,0x8e,0x90,0x92,0x94,0x96,0x98,0x9a,0x9c,0x9e,
   0xa0,0xa2,0xa4,0xa6,0xa8,0xaa,0xac,0xae,0xb0,0xb2,0xb4,0xb6,0xb8,0xba,0xbc,0xbe,
   0xc0,0xc2,0xc4,0xc6,0xc8,0xca,0xcc,0xce,0xd0,0xd2,0xd4,0xd6,0xd8,0xda,0xdc,0xde,
   0xe0,0xe2,0xe4,0xe6,0xe8,0xea,0xec,0xee,0xf0,0xf2,0xf4,0xf6,0xf8,0xfa,0xfc,0xfe,
   0x1b,0x19,0x1f,0x1d,0x13,0x11,0x17,0x15,0x0b,0x09,0x0f,0x0d,0x03,0x01,0x07,0x05,
   0x3b,0x39,0x3f,0x3d,0x33,0x31,0x37,0x35,0x2b,0x29,0x2f,0x2d,0x23,0x21,0x27,0x25,
   0x5b,0x59,0x5f,0x5d,0x53,0x51,0x57,0x55,0x4b,0x49,0x4f,0x4d,0x43,0x41,0x47,0x45,
   0x7b,0x79,0x7f,0x7d,0x73,0x71,0x77,0x75,0x6b,0x69,0x6f,0x6d,0x63,0x61,0x67,0x65,
   0x9b,0x99,0x9f,0x9d,0x93,0x91,0x97,0x95,0x8b,0x89,0x8f,0x8d,0x83,0x81,0x87,0x85,
   0xbb,0xb9,0xbf,0xbd,0xb3,0xb1,0xb7,0xb5,0xab,0xa9,0xaf,0xad,0xa3,0xa1,0xa7,0xa5,
   0xdb,0xd9,0xdf,0xdd,0xd3,0xd1,0xd7,0xd5,0xcb,0xc9,0xcf,0xcd,0xc3,0xc1,0xc7,0xc5,
   0xfb,0xf9,0xff,0xfd,0xf3,0xf1,0xf7,0xf5,0xeb,0xe9,0xef,0xed,0xe3,0xe1,0xe7,0xe5
};

const uint8_t mul3[] = {
   0x00,0x03,0x06,0x05,0x0c,0x0f,0x0a,0x09,0x18,0x1b,0x1e,0x1d,0x14,0x17,0x12,0x11,
   0x30,0x33,0x36,0x35,0x3c,0x3f,0x3a,0x39,0x28,0x2b,0x2e,0x2d,0x24,0x27,0x22,0x21,
   0x60,0x63,0x66,0x65,0x6c,0x6f,0x6a,0x69,0x78,0x7b,0x7e,0x7d,0x74,0x77,0x72,0x71,
   0x50,0x53,0x56,0x55,0x5c,0x5f,0x5a,0x59,0x48,0x4b,0x4e,0x4d,0x44,0x47,0x42,0x41,
   0xc0,0xc3,0xc6,0xc5,0xcc,0xcf,0xca,0xc9,0xd8,0xdb,0xde,0xdd,0xd4,0xd7,0xd2,0xd1,
   0xf0,0xf3,0xf6,0xf5,0xfc,0xff,0xfa,0xf9,0xe8,0xeb,0xee,0xed,0xe4,0xe7,0xe2,0xe1,
   0xa0,0xa3,0xa6,0xa5,0xac,0xaf,0xaa,0xa9,0xb8,0xbb,0xbe,0xbd,0xb4,0xb7,0xb2,0xb1,
   0x90,0x93,0x96,0x95,0x9c,0x9f,0x9a,0x99,0x88,0x8b,0x8e,0x8d,0x84,0x87,0x82,0x81,
   0x9b,0x98,0x9d,0x9e,0x97,0x94,0x91,0x92,0x83,0x80,0x85,0x86,0x8f,0x8c,0x89,0x8a,
   0xab,0xa8,0xad,0xae,0xa7,0xa4,0xa1,0xa2,0xb3,0xb0,0xb5,0xb6,0xbf,0xbc,0xb9,0xba,
   0xfb,0xf8,0xfd,0xfe,0xf7,0xf4,0xf1,0xf2,0xe3,0xe0,0xe5,0xe6,0xef,0xec,0xe9,0xea,
   0xcb,0xc8,0xcd,0xce,0xc7,0xc4,0xc1,0xc2,0xd3,0xd0,0xd5,0xd6,0xdf,0xdc,0xd9,0xda,
   0x5b,0x58,0x5d,0x5e,0x57,0x54,0x51,0x52,0x43,0x40,0x45,0x46,0x4f,0x4c,0x49,0x4a,
   0x6b,0x68,0x6d,0x6e,0x67,0x64,0x61,0x62,0x73,0x70,0x75,0x76,0x7f,0x7c,0x79,0x7a,
   0x3b,0x38,0x3d,0x3e,0x37,0x34,0x31,0x32,0x23,0x20,0x25,0x26,0x2f,0x2c,0x29,0x2a,
   0x0b,0x08,0x0d,0x0e,0x07,0x04,0x01,0x02,0x13,0x10,0x15,0x16,0x1f,0x1c,0x19,0x1a
};

void FillLookupTable(LookupTableAddr addr){
   VersatMemoryCopy(addr.addr,(void*) sbox,256 * sizeof(uint8_t));
}

void FillMul(LookupTableAddr addr,const uint8_t* mem,int size){
   VersatMemoryCopy(addr.addr,(void*) mem,size * sizeof(uint8_t)); // (int*) 
}

void FillRow(DoRowAddr addr){
   FillMul(addr.mul2_0,mul2,ARRAY_SIZE(mul2));
   FillMul(addr.mul2_1,mul2,ARRAY_SIZE(mul2));
   FillMul(addr.mul3_0,mul3,ARRAY_SIZE(mul3));
   FillMul(addr.mul3_1,mul3,ARRAY_SIZE(mul3));
}

void FillFirstLineKey(FirstLineKeyAddr addr){
   FillLookupTable(addr.b0);
   FillLookupTable(addr.b1);
}

void FillFourthLineKey(FourthLineKeyAddr addr){
   FillLookupTable(addr.b0);
   FillLookupTable(addr.b1);
}

void FillMixColumns(MixColumnsAddr addr){
   FillRow(addr.d0);
   FillRow(addr.d1);
   FillRow(addr.d2);
   FillRow(addr.d3);
}

void FillSBox(SBoxAddr addr){
   FillLookupTable(addr.s0);
   FillLookupTable(addr.s1);
   FillLookupTable(addr.s2);
   FillLookupTable(addr.s3);
   FillLookupTable(addr.s4);
   FillLookupTable(addr.s5);
   FillLookupTable(addr.s6);
   FillLookupTable(addr.s7);
}

void FillKeySchedule(KeySchedule256Addr addr){
   FillFirstLineKey(addr.s);
   FillFourthLineKey(addr.q);
}

void FillMainRound(MainRoundAddr addr){
   FillSBox(addr.subBytes);
   FillMixColumns(addr.mixColumns);
}

void FillRoundPairAndKey(RoundPairAndKeyAddr addr){
   FillMainRound(addr.round1);
   FillMainRound(addr.round2);
   FillKeySchedule(addr.key);
}

void byte_to_int(uint8_t *in, int *out, int size) {
   int i = 0;
   for(i=0; i<size; i++) {
      out[i] = (int) in[i];
   }
   return;
}

void int_to_byte(int *in, uint8_t *out, int size) {
   int i = 0;
   for(i=0; i<size; i++) {
      out[i] = (uint8_t) (in[i] & 0x0FF);
   }
   return;
}

void InitVersatAES() {
   SHA_AES_SimpleConfig* config = (SHA_AES_SimpleConfig*) accelConfig;
   AES256WithIterativeConfig* aes = &config->simple.AES256WithIterative;

   SHA_AES_SimpleAddr addr = ACCELERATOR_TOP_ADDR_INIT;

   FillRoundPairAndKey(addr.simple.mk0.roundPairAndKey);
   FillMainRound(addr.simple.round0);
   FillKeySchedule(addr.simple.key6);
   FillSBox(addr.simple.subBytes);

   aes->rcon0.constant = 0x01;
   aes->rcon1.constant = 0x02;
   aes->rcon2.constant = 0x04;
   aes->rcon3.constant = 0x08;
   aes->rcon4.constant = 0x10;
   aes->rcon5.constant = 0x20;
   aes->rcon6.constant = 0x40;
}

void VersatAES(uint8_t *result, uint8_t *cypher, uint8_t *key) {
   int cypher_int[AES_BLK_SIZE] = {0};
   int key_int[AES_KEY_SIZE] = {0};
   int result_int[AES_BLK_SIZE] = {0};

   byte_to_int(cypher, cypher_int, AES_BLK_SIZE);
   byte_to_int(key, key_int, AES_KEY_SIZE);

   int i = 0;
   for(i = 0; i < AES_BLK_SIZE; i++){
      SimpleInputStart[i] = cypher_int[i];
   }
   for(i = 0; i < AES_KEY_SIZE; i++){
      SimpleInputStart[i+AES_BLK_SIZE] = key_int[i];
   }

   RunAccelerator(1);

   for(i = 0; i < AES_BLK_SIZE; i++){
      result_int[i] = SimpleOutputStart[i];
   }

   int_to_byte(result_int, result, AES_BLK_SIZE);

   return;
}

static char HexToInt(char ch){
   if('0' <= ch && ch <= '9'){
      return (ch - '0');
   } else if('a' <= ch && ch <= 'f'){
      return ch - 'a' + 10;
   } else if('A' <= ch && ch <= 'F'){
      return ch - 'A' + 10;
   } else {
      printf("Error, invalid character inside hex string:%c",ch);
      return 0;
   }
}

// Make sure that buffer is capable of storing the whole thing. Returns number of bytes inserted
int HexStringToHex(char* buffer,const char* str){
   int inserted = 0;
   for(int i = 0; ; i += 2){
      char upper = str[i];
      char lower = str[i+1];

      if(upper == '\0' || lower == '\0'){
         if(upper != '\0') printf("Warning: HexString was not divisible by 2\n");
         break;
      }   

      buffer[inserted++] = HexToInt(upper) * 16 + HexToInt(lower);
   }

   return inserted;
}

static uint32_t initialStateValues[] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
static uint32_t kConstants0[] = {0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174};
static uint32_t kConstants1[] = {0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967};
static uint32_t kConstants2[] = {0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070};
static uint32_t kConstants3[] = {0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};

static uint32_t* kConstants[4] = {kConstants0,kConstants1,kConstants2,kConstants3};

// GLOBALS
static bool initVersat = false;

static void store_bigendian_32(uint8_t *x, uint32_t u) {
   x[3] = (uint8_t) u;
   u >>= 8;
   x[2] = (uint8_t) u;
   u >>= 8;
   x[1] = (uint8_t) u;
   u >>= 8;
   x[0] = (uint8_t) u;
}

static size_t versat_crypto_hashblocks_sha256(const uint8_t *in, size_t inlen) {
   while (inlen >= 64) {
      ACCEL_TOP_simple_MemRead_row_ext_addr = (iptr) in;

      // Loads data + performs work
      RunAccelerator(1);

      if(!initVersat){
         VersatUnitWrite(TOP_simple_State_s0_reg_addr,0,initialStateValues[0]);
         VersatUnitWrite(TOP_simple_State_s1_reg_addr,0,initialStateValues[1]);
         VersatUnitWrite(TOP_simple_State_s2_reg_addr,0,initialStateValues[2]);
         VersatUnitWrite(TOP_simple_State_s3_reg_addr,0,initialStateValues[3]);
         VersatUnitWrite(TOP_simple_State_s4_reg_addr,0,initialStateValues[4]);
         VersatUnitWrite(TOP_simple_State_s5_reg_addr,0,initialStateValues[5]);
         VersatUnitWrite(TOP_simple_State_s6_reg_addr,0,initialStateValues[6]);
         VersatUnitWrite(TOP_simple_State_s7_reg_addr,0,initialStateValues[7]);
         initVersat = true;
      }

      in += 64;
      inlen -= 64;
   }

   return inlen;
}

void VersatSHA(uint8_t *out, const uint8_t *in, size_t inlen) {
   uint8_t padded[128];
   uint64_t bytes = inlen;

   versat_crypto_hashblocks_sha256(in, inlen);
   in += inlen;
   inlen &= 63;
   in -= inlen;

   for (size_t i = 0; i < inlen; ++i) {
      padded[i] = in[i];
   }
   padded[inlen] = 0x80;

   if (inlen < 56) {
      for (size_t i = inlen + 1; i < 56; ++i) {
         padded[i] = 0;
      
    }  padded[56] = (uint8_t) (bytes >> 53);
      padded[57] = (uint8_t) (bytes >> 45);
      padded[58] = (uint8_t) (bytes >> 37);
      padded[59] = (uint8_t) (bytes >> 29);
      padded[60] = (uint8_t) (bytes >> 21);
      padded[61] = (uint8_t) (bytes >> 13);
      padded[62] = (uint8_t) (bytes >> 5);
      padded[63] = (uint8_t) (bytes << 3);
      versat_crypto_hashblocks_sha256(padded, 64);
   } else {
      for (size_t i = inlen + 1; i < 120; ++i) {
         padded[i] = 0;
      }
      padded[120] = (uint8_t) (bytes >> 53);
      padded[121] = (uint8_t) (bytes >> 45);
      padded[122] = (uint8_t) (bytes >> 37);
      padded[123] = (uint8_t) (bytes >> 29);
      padded[124] = (uint8_t) (bytes >> 21);
      padded[125] = (uint8_t) (bytes >> 13);
      padded[126] = (uint8_t) (bytes >> 5);
      padded[127] = (uint8_t) (bytes << 3);
      versat_crypto_hashblocks_sha256(padded, 128);
   }

   RunAccelerator(1);

   store_bigendian_32(&out[0*4],(uint32_t) VersatUnitRead(TOP_simple_State_s0_reg_addr,0));
   store_bigendian_32(&out[1*4],(uint32_t) VersatUnitRead(TOP_simple_State_s1_reg_addr,0));
   store_bigendian_32(&out[2*4],(uint32_t) VersatUnitRead(TOP_simple_State_s2_reg_addr,0));
   store_bigendian_32(&out[3*4],(uint32_t) VersatUnitRead(TOP_simple_State_s3_reg_addr,0));
   store_bigendian_32(&out[4*4],(uint32_t) VersatUnitRead(TOP_simple_State_s4_reg_addr,0));
   store_bigendian_32(&out[5*4],(uint32_t) VersatUnitRead(TOP_simple_State_s5_reg_addr,0));
   store_bigendian_32(&out[6*4],(uint32_t) VersatUnitRead(TOP_simple_State_s6_reg_addr,0));
   store_bigendian_32(&out[7*4],(uint32_t) VersatUnitRead(TOP_simple_State_s7_reg_addr,0));

   initVersat = false; // At the end of each run, reset
}

void InitVersatSHA(){
   SHA_AES_SimpleConfig* config = (SHA_AES_SimpleConfig*) accelConfig;
   SHAConfig* sha = &config->simple.SHA;

   ConfigureSimpleVRead(&sha->MemRead,16,nullptr);

   ACCEL_Constants_mem_iterA = 1;
   ACCEL_Constants_mem_incrA = 1;
   ACCEL_Constants_mem_perA = 16;
   ACCEL_Constants_mem_dutyA = 16;
   ACCEL_Constants_mem_startA = 0;
   ACCEL_Constants_mem_shiftA = 0;

   for(int ii = 0; ii < 16; ii++){
      VersatUnitWrite(TOP_simple_cMem0_mem_addr,ii,kConstants[0][ii]);
   }
   for(int ii = 0; ii < 16; ii++){
      VersatUnitWrite(TOP_simple_cMem1_mem_addr,ii,kConstants[1][ii]);
   }
   for(int ii = 0; ii < 16; ii++){
      VersatUnitWrite(TOP_simple_cMem2_mem_addr,ii,kConstants[2][ii]);
   }
   for(int ii = 0; ii < 16; ii++){
      VersatUnitWrite(TOP_simple_cMem3_mem_addr,ii,kConstants[3][ii]);
   }

   ACCEL_TOP_simple_Swap_enabled = 1;
}

char GetHexadecimalChar(unsigned char value){
  if(value < 10){
    return '0' + value;
  } else{
    return 'a' + (value - 10);
  }
}

char* GetHexadecimal(const char* text,char* buffer,int str_size){
  int i = 0;
  unsigned char* view = (unsigned char*) text;
  for(; i< str_size; i++){
    buffer[i*2] = GetHexadecimalChar(view[i] / 16);
    buffer[i*2+1] = GetHexadecimalChar(view[i] % 16);
  }

  buffer[i*2] = '\0';

  return buffer;
}

/*
  This file is for public-key generation
*/

#include "benes.h"
#include "controlbits.h"
#include "gf.h"
#include "params.h"
#include "pk_gen.h"
#include "root.h"
#include "util.h"
#include "arena.h"

#define VERSAT
//#define SIMULATED_VERSAT

void VersatLineXOR(uint8_t* out, uint8_t *mat, uint8_t *row, int n_cols, uint8_t mask) {
   static bool once = true;
   if(once){
      once = false;
#ifdef SIMULATED_VERSAT
       printf("Simulating versat\n");
#else
       printf("Using versat\n");
#endif
   }

   uint32_t mask_int = (mask) | (mask << 8) | (mask << 8*2) | (mask << 8*3);
   uint32_t *mat_int = (uint32_t*) mat;
   uint32_t *out_int = (uint32_t*) out;
   uint32_t *row_int = (uint32_t*) row;
   int n_cols_int = (n_cols >> 2);

#ifdef SIMULATED_VERSAT
   for(int i = 0; i < n_cols_int; i++){
        uint32_t a = row_int[i] & mask_int;
        uint32_t b = mat_int[i] ^ a;
        out_int[i] = b;
   }
#else
   SHA_AES_SimpleConfig* config = (SHA_AES_SimpleConfig*) accelConfig;
   SHA_AES_SimpleAddr addr = ACCELERATOR_TOP_ADDR_INIT;
   VectorLikeOperationConfig* vec = &config->simple.VectorLikeOperation;

   ConfigureSimpleVRead(&vec->row, n_cols_int, (int*) row_int);
   ConfigureSimpleMemoryAndCopyData(&vec->mat,n_cols_int,0,addr.simple.mat,(int*) mat_int);

   vec->mask.constant = mask_int;
   ConfigureMemoryReceive(&vec->output, n_cols_int);

   RunAccelerator(2);

   for (int i = 0; i < n_cols_int; i++){
        out_int[i] = VersatUnitRead((iptr) addr.simple.output.addr,i);
   }
#endif
}

/* input: secret key sk */
/* output: public key pk */
int PQCLEAN_MCELIECE348864_CLEAN_pk_gen(uint8_t *pk, uint32_t *perm, const uint8_t *sk) {
    int i, j, k;
    int row, c;

    int mark = MarkArena();
    uint64_t *buf = (uint64_t*) PushBytes((1 << GFBITS)*sizeof(uint64_t));

    uint8_t *mat = (uint8_t*) PushBytes(( GFBITS * SYS_T )*( SYS_N / 8 )*sizeof(uint8_t));
    uint8_t mask;
    uint8_t b;

    gf *g = (gf*) PushBytes((SYS_T + 1)*sizeof(gf)); // Goppa polynomial
    gf *L = (gf*) PushBytes((SYS_N)*sizeof(gf)); // support
    gf *inv = (gf*) PushBytes((SYS_N)*sizeof(gf));

    //

    g[ SYS_T ] = 1;

    for (i = 0; i < SYS_T; i++) {
        g[i] = PQCLEAN_MCELIECE348864_CLEAN_load2(sk);
        g[i] &= GFMASK;
        sk += 2;
    }

    for (i = 0; i < (1 << GFBITS); i++) {
        buf[i] = perm[i];
        buf[i] <<= 31;
        buf[i] |= i;
    }

    PQCLEAN_MCELIECE348864_CLEAN_sort_63b(1 << GFBITS, buf);

    for (i = 0; i < (1 << GFBITS); i++) {
        perm[i] = buf[i] & GFMASK;
    }

    for (i = 0; i < SYS_N;         i++) {
        L[i] = PQCLEAN_MCELIECE348864_CLEAN_bitrev((gf)perm[i]);
    }

    // filling the matrix

    PQCLEAN_MCELIECE348864_CLEAN_root(inv, g, L);

    for (i = 0; i < SYS_N; i++) {
        inv[i] = PQCLEAN_MCELIECE348864_CLEAN_gf_inv(inv[i]);
    }

    for (i = 0; i < PK_NROWS; i++) {
        for (j = 0; j < SYS_N / 8; j++) {
            // mat[i][j] = 0;
            mat[i*(SYS_N/8)+j] = 0;
        }
    }

    for (i = 0; i < SYS_T; i++) {
        for (j = 0; j < SYS_N; j += 8) {
            for (k = 0; k < GFBITS;  k++) {
                b  = (inv[j + 7] >> k) & 1;
                b <<= 1;
                b |= (inv[j + 6] >> k) & 1;
                b <<= 1;
                b |= (inv[j + 5] >> k) & 1;
                b <<= 1;
                b |= (inv[j + 4] >> k) & 1;
                b <<= 1;
                b |= (inv[j + 3] >> k) & 1;
                b <<= 1;
                b |= (inv[j + 2] >> k) & 1;
                b <<= 1;
                b |= (inv[j + 1] >> k) & 1;
                b <<= 1;
                b |= (inv[j + 0] >> k) & 1;

                mat[( i * GFBITS + k)*(SYS_N/8) + (j / 8) ] = b;
            }
        }

        for (j = 0; j < SYS_N; j++) {
            inv[j] = PQCLEAN_MCELIECE348864_CLEAN_gf_mul(inv[j], L[j]);
        }

    }

    // gaussian elimination
    for (i = 0; i < (GFBITS * SYS_T + 7) / 8; i++) {
        for (j = 0; j < 8; j++) {
            row = i * 8 + j;

            if (row >= GFBITS * SYS_T) {
                break;
            }

            for (k = row + 1; k < GFBITS * SYS_T; k++) {
                mask = mat[ row*(SYS_N/8) + i ] ^ mat[ k*(SYS_N/8) + i ];
                mask >>= j;
                mask &= 1;
                mask = -mask;

#ifdef VERSAT
                if (mask != 0){
                    VersatLineXOR(&(mat[row*(SYS_N/8)+0]), &(mat[row*(SYS_N/8)+0]), &(mat[k*(SYS_N/8)+0]), SYS_N / 8, mask);
                }
#else
                for (c = 0; c < SYS_N / 8; c++) {
                    mat[  row*(SYS_N/8) + c ] ^= mat[ k*(SYS_N/8) + c ] & mask;
                }
#endif
            }

            if ( ((mat[ row*(SYS_N/8) + i ] >> j) & 1) == 0 ) { // return if not systematic
                PopArena(mark);
                return -1;
            }

            for (k = 0; k < GFBITS * SYS_T; k++) {
                if (k != row) {
                    mask = mat[ k*(SYS_N/8) + i ] >> j;
                    mask &= 1;
                    mask = -mask;

#ifdef VERSAT
                    if (mask != 0){
                        VersatLineXOR(&(mat[k*(SYS_N/8)+0]), &(mat[k*(SYS_N/8)+0]), &(mat[row*(SYS_N/8)+0]), SYS_N / 8, mask);
                    }
#else                    
                    for (c = 0; c < SYS_N / 8; c++) {
                       mat[ k*(SYS_N/8) + c ] ^= mat[ row*(SYS_N/8) + c ] & mask;
                    }
#endif
                }
            }
        }
    }

    for (i = 0; i < PK_NROWS; i++) {
        memcpy(pk + i * PK_ROW_BYTES, &(mat[i*(SYS_N/8)]) + PK_NROWS / 8, PK_ROW_BYTES);
    }

    PopArena(mark);
    return 0;
}

void SingleTest(){
   // SHA
   {
   unsigned char msg_64[] = { 0x5a, 0x86, 0xb7, 0x37, 0xea, 0xea, 0x8e, 0xe9, 0x76, 0xa0, 0xa2, 0x4d, 0xa6, 0x3e, 0x7e, 0xd7, 0xee, 0xfa, 0xd1, 0x8a, 0x10, 0x1c, 0x12, 0x11, 0xe2, 0xb3, 0x65, 0x0c, 0x51, 0x87, 0xc2, 0xa8, 0xa6, 0x50, 0x54, 0x72, 0x08, 0x25, 0x1f, 0x6d, 0x42, 0x37, 0xe6, 0x61, 0xc7, 0xbf, 0x4c, 0x77, 0xf3, 0x35, 0x39, 0x03, 0x94, 0xc3, 0x7f, 0xa1, 0xa9, 0xf9, 0xbe, 0x83, 0x6a, 0xc2, 0x85, 0x09 };
   static const int HASH_SIZE = (256/8);
   
   InitVersatSHA();

   unsigned char digest[256];
   for(int i = 0; i < 256; i++){
      digest[i] = 0;
   }

   VersatSHA(digest,msg_64,64);

   char buffer[2048];
   GetHexadecimal((char*) digest,buffer, HASH_SIZE);
   printf("%s\n","42e61e174fbb3897d6dd6cef3dd2802fe67b331953b06114a65c772859dfc1aa"); 
   printf("%s\n",buffer);
   }

   // AES
   {
   uint8_t key[128] = {};
   uint8_t plain[128] = {};

   int keyIndex = HexStringToHex((char*) key,"cc22da787f375711c76302bef0979d8eddf842829c2b99ef3dd04e23e54cc24b");
   int plainIndex = HexStringToHex((char*) plain,"ccc62c6b0a09a671d64456818db29a4d");

   printf("keyIndex: %d\n",keyIndex);
   printf("plainIndex: %d\n",plainIndex);

   uint8_t result[AES_BLK_SIZE] = {};

   InitVersatAES();

   VersatAES(result,plain,key);

   char buffer[2048];
   GetHexadecimal((char*) result,buffer, AES_BLK_SIZE);
   printf("%s\n",buffer);
   printf("%s\n","df8634ca02b13a125b786e1dce90658b");
   }

   // VectorLikeOperation (McEliece base)
   {
      SHA_AES_SimpleConfig* config = (SHA_AES_SimpleConfig*) accelConfig;

      SHA_AES_SimpleAddr addr = ACCELERATOR_TOP_ADDR_INIT;
      int testRow[8] = {0x19,0x2a,0x3b,0x4c,0x5d,0x6e,0x7f,0x80};
      int testMem[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};

      VectorLikeOperationConfig* vec = &config->simple.VectorLikeOperation;

      vec->mask.constant = 0x0f;

      ConfigureSimpleVRead(&vec->row,8,testRow);      
      ConfigureSimpleMemoryAndCopyData(&vec->mat,8,0,addr.simple.mat,testMem);
      ConfigureMemoryReceive(&vec->output,8);

      int result[8] = {};
      RunAccelerator(2);

      // Must likely does not work because of testMem;
      for(int i = 0; i < 8; i++){
         result[i] = VersatUnitRead((iptr) addr.simple.output.addr,i);
         printf("%x\n",result[i]);
      }
   }
}
