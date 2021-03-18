/* based on SipHash reference C implementation (CC0) */
#include <stddef.h>
#include <stdint.h>

#define cROUNDS 2
#define dROUNDS 4

#define ROTL(x,b) (uint64_t)( ((x) << (b)) | ( (x) >> (64 - (b))) )

#define U8TO64_LE(p)            \
  (((uint64_t)((p)[0])      ) | \
   ((uint64_t)((p)[1]) <<  8) | \
   ((uint64_t)((p)[2]) << 16) | \
   ((uint64_t)((p)[3]) << 24) | \
   ((uint64_t)((p)[4]) << 32) | \
   ((uint64_t)((p)[5]) << 40) | \
   ((uint64_t)((p)[6]) << 48) | \
   ((uint64_t)((p)[7]) << 56))

#define SIPROUND                                        \
  do {                                                  \
    v0 += v1; v1=ROTL(v1,13); v1 ^= v0; v0=ROTL(v0,32); \
    v2 += v3; v3=ROTL(v3,16); v3 ^= v2;                 \
    v0 += v3; v3=ROTL(v3,21); v3 ^= v0;                 \
    v2 += v1; v1=ROTL(v1,17); v1 ^= v2; v2=ROTL(v2,32); \
  } while(0)

uint64_t siphash(const uint8_t *in, size_t len, const uint8_t *k)
{
  /* "somepseudorandomlygeneratedbytes" */
  uint64_t v0 = 0x736f6d6570736575ULL;
  uint64_t v1 = 0x646f72616e646f6dULL;
  uint64_t v2 = 0x6c7967656e657261ULL;
  uint64_t v3 = 0x7465646279746573ULL;
  uint64_t b;
  uint64_t k0 = U8TO64_LE( k );
  uint64_t k1 = U8TO64_LE( k + 8 );
  uint64_t m;
  int i;
  const uint8_t *end = in + len - len%8;
  const int left = len%8;
  b = (uint64_t)len << 56;
  v3 ^= k1;
  v2 ^= k0;
  v1 ^= k1;
  v0 ^= k0;

  for ( ; in != end; in += 8 )
  {
    m = U8TO64_LE( in );
    v3 ^= m;
    for( i=0; i<cROUNDS; ++i ) SIPROUND;
    v0 ^= m;
  }

  switch( left )
  {
  case 7: b |= ( ( uint64_t )in[ 6] )  << 48;
  case 6: b |= ( ( uint64_t )in[ 5] )  << 40;
  case 5: b |= ( ( uint64_t )in[ 4] )  << 32;
  case 4: b |= ( ( uint64_t )in[ 3] )  << 24;
  case 3: b |= ( ( uint64_t )in[ 2] )  << 16;
  case 2: b |= ( ( uint64_t )in[ 1] )  <<  8;
  case 1: b |= ( ( uint64_t )in[ 0] ); break;
  case 0: break;
  }

  v3 ^= b;
  for( i=0; i<cROUNDS; ++i ) SIPROUND;
  v0 ^= b;
  v2 ^= 0xff;
  for( i=0; i<dROUNDS; ++i ) SIPROUND;

  return v0 ^ v1 ^ v2  ^ v3;
}
