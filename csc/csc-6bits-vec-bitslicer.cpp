// Copyright distributed.net 1997 - All Rights Reserved
// For use in distributed.net projects only.
// Any other distribution or use of this source violates copyright.
//
// $Log: csc-6bits-vec-bitslicer.cpp,v $
// Revision 1.3  2000/06/02 06:32:56  jlawson
// sync, copy files from release branch to head
//
// Revision 1.1.2.1  1999/12/09 04:56:49  sampo
// first few files of the altivec bitslicer for CSC.  note: this is known to be broken!  it will not work!  This is just the start.
// things that I still have to do:
// 1) write conversion routines for keyhi/keylo that take 128 bit vectors
// 2) move keyhi/keylo over to vectors instead of ulongs
// 3) any other errors made by code that assumes 64bit is the maximum bitslice?
//
// Revision 1.3  1999/11/01 17:25:51  cyp
// sync from release
//
// Revision 1.1.2.6  1999/11/01 17:23:23  cyp
// renamed transX(...) to csc_transX(...) to avoid potential (future) symbol
// collisions.
//
// Revision 1.1.2.5  1999/10/30 14:59:01  remi
// cosmetic improvements.
//
// Revision 1.1.2.4  1999/10/26 20:48:52  remi
// Moved tp1[] and tp2[] to the 16-byte aligned memory pool.
//
// Revision 1.1.2.3  1999/10/24 23:54:53  remi
// Use Problem::core_membuffer instead of stack for CSC cores.
// Align frequently used memory to 16-byte boundary in CSC cores.
//
// Revision 1.1.2.2  1999/10/08 00:07:01  cyp
// made (mostly) all extern "C" {}
//
// Revision 1.1.2.1  1999/10/07 18:41:14  cyp
// sync'd from head
//
// Revision 1.1  1999/07/23 02:43:05  fordbr
// CSC cores added
//
//

#if (!defined(lint) && defined(__showids__))
const char * PASTE(csc_6bits_bitslicer_,CSC_SUFFIX) (void) {
return "@(#)$Id: csc-6bits-vec-bitslicer.cpp,v 1.3 2000/06/02 06:32:56 jlawson Exp $"; }
#endif

// ------------------------------------------------------------------
// version 6 bits : 08/04 17:43
//
// -O2 == -fomit-frame-pointer -fstrict-aliasing -fno-gcse -O2 -m486
// -O3 == -fomit-frame-pointer -fstrict-aliasing -fno-gcse -O3 -mcpu=pentium
//
//        -O2    -O3
// 486  :  33     ..  egcs 2.93.17 19990405
// K5   :  67     60      //
// K6   : 220    300      //
// K6-2 : 286    394      //
// K6-2 : 255    247  gcc 2.7.2.1
// alpha:  80     ..  egcs 2.93.?? 19990321 (-mcpu=ev5)
//
#ifdef __cplusplus
extern "C" {
vulong
PASTE(vec_cscipher_bitslicer_,CSC_SUFFIX)
( ulong key[2][64], const u8 keyB[8], const vulong msg[64], const vulong cipher[64], char *membuffer );
}
#endif

/*
  Memory usage :
  	- key	    :  512 bytes -+
	- plain     :  256 bytes  | allocated in csc-6bit-driver.cpp
	- cipher    :  256 bytes -+
  	- subkey    : 2816 bytes
	- cfr       :  256 bytes
	- tp1+tp2   :  256 bytes
	- totwiddle :  696 bytes
	- tcp+tep   : 2816 bytes
	- loc.var.  :   64 bytes
		      ----
                      7928 bytes (or 15856 bytes on a 64-bit cpu)
 */

//#include <stdio.h>

vulong
PASTE(vec_cscipher_bitslicer_,CSC_SUFFIX)
( ulong key[2][64], const u8 keyB[8], const vulong msg[64], const vulong cipher[64], char *membuffer )
{
  vulong cfr[64];
  //ulong (*cfr)[64] = (ulong(*)[64])membuffer;
  //membuffer += (sizeof(*cfr) + 15) & 0xFFFFFFF0;

  //ulong subkey[9+2][64];
  ulong (*subkey)[9+2][64] = (ulong (*)[9+2][64])membuffer;
  membuffer += (sizeof(*subkey) + 15) & 0xFFFFFFF0;

  //ulong *totwiddle[6][7*(1+3)+1];
  vulong *(*totwiddle)[6][7*(1+3)+1] = (vulong *(*)[6][7*(1+3)+1])membuffer;
  membuffer += (sizeof(*totwiddle) + 15) & 0xFFFFFFF0;

  //ulong tp1[4][8], tp2[4][8];
  vulong (*tp1)[4][8] = (vulong (*)[4][8])membuffer;
  membuffer += (sizeof(*tp1) + 15) & 0xFFFFFFF0;
  vulong (*tp2)[4][8] = (vulong (*)[4][8])membuffer;


  vulong *skp;  // subkey[n]
  vulong *skp1; // subkey[n-1]
  const vulong *tcp; // pointer to tabc[] (bitslice values of c0..c8)
  const vulong *tep; // pointer to tabe[] (bitslice values of e and e')
  // temp. variables used in the encryption process
  vulong x0,x1,x2,x3,x4,x5,x6,x7, y1,y3,y5,y7;
  vulong xy56, xy34, xy12, xy70;

  //printf( "subkey=%p cfr=%p tp1=%p tp2=%p\n", subkey, cfr, tp1, tp2 );
  //exit( 0 );

#define APPLY_MP0(adr, adl)			    \
  csc_transP( (*tp1)[adr/16][7], (*tp1)[adr/16][6], \
              (*tp1)[adr/16][5], (*tp1)[adr/16][4], \
	      (*tp1)[adr/16][3], (*tp1)[adr/16][2], \
	      (*tp1)[adr/16][1], (*tp1)[adr/16][0], \
	  cfr[adl+7], cfr[adl+6], cfr[adl+5], cfr[adl+4],			\
	  cfr[adl+3], cfr[adl+2], cfr[adl+1], cfr[adl+0] );			\
  csc_transP( (*tp2)[adr/16][7], (*tp2)[adr/16][6], \
              (*tp2)[adr/16][5], (*tp2)[adr/16][4], \
              (*tp2)[adr/16][3], (*tp2)[adr/16][2], \
	      (*tp2)[adr/16][1], (*tp2)[adr/16][0], \
	  cfr[adr+7], cfr[adr+6], cfr[adr+5], cfr[adr+4],			\
	  cfr[adr+3], cfr[adr+2], cfr[adr+1], cfr[adr+0] )

#define APPLY_Ms(adr, adl)						\
  x0 = vec_xor(cfr[adl+0],(skp[0+8] = vec_xor(skp[0+8],skp[0+8-128])));				\
  x1 = vec_xor(cfr[adl+1],(skp[1+8] = vec_xor(skp[1+8],skp[1+8-128])));				\
  x2 = vec_xor(cfr[adl+2],(skp[2+8] = vec_xor(skp[2+8],skp[2+8-128])));				\
  x3 = vec_xor(cfr[adl+3],(skp[3+8] = vec_xor(skp[3+8],skp[3+8-128])));				\
  x4 = vec_xor(cfr[adl+4],(skp[4+8] = vec_xor(skp[4+8],skp[4+8-128])));				\
  x5 = vec_xor(cfr[adl+5],(skp[5+8] = vec_xor(skp[5+8],skp[5+8-128])));				\
  x6 = vec_xor(cfr[adl+6],(skp[6+8] = vec_xor(skp[6+8],skp[6+8-128])));				\
  x7 = vec_xor(cfr[adl+7],(skp[7+8] = vec_xor(skp[7+8],skp[7+8-128])));				\
  csc_transP(                                                           \
      vec_xor(x7,(y7   =      vec_xor(cfr[adr+7],(skp[7] = vec_xor(skp[7],skp[7-128]))))),	\
	  vec_xor(x6,(xy56 = vec_xor(x5,vec_xor(cfr[adr+6],(skp[6] = vec_xor(skp[6],skp[6-128])))))),	\
	  vec_xor(x5,(y5   =      vec_xor(cfr[adr+5],(skp[5] = vec_xor(skp[5],skp[5-128]))))),	\
	  vec_xor(x4,(xy34 = vec_xor(x3,vec_xor(cfr[adr+4],(skp[4] = vec_xor(skp[4],skp[4-128])))))),	\
	  vec_xor(x3,(y3   =      vec_xor(cfr[adr+3],(skp[3] = vec_xor(skp[3],skp[3-128]))))),	\
	  vec_xor(x2,(xy12 = vec_xor(x1,vec_xor(cfr[adr+2],(skp[2] = vec_xor(skp[2],skp[2-128])))))),	\
	  vec_xor(x1,(y1   =      vec_xor(cfr[adr+1],(skp[1] = vec_xor(skp[1],skp[1-128]))))),	\
	  vec_xor(x0,(xy70 = vec_xor(x7,vec_xor(cfr[adr+0],(skp[0] = vec_xor(skp[0],skp[0-128])))))),	\
	  cfr[adl+7], cfr[adl+6], cfr[adl+5], cfr[adl+4],		\
	  cfr[adl+3], cfr[adl+2], cfr[adl+1], cfr[adl+0] );		\
  csc_transP(                                                           \
      vec_xor(x6,y7), xy56, vec_xor(x4,y5), xy34,					\
	  vec_xor(x2,y3), xy12, vec_xor(x0,y1), xy70,					\
	  cfr[adr+7], cfr[adr+6], cfr[adr+5], cfr[adr+4],		\
	  cfr[adr+3], cfr[adr+2], cfr[adr+1], cfr[adr+0] );		\
  skp += 16;

#define APPLY_Me(adr, adl)						\
  x0 = vec_xor(cfr[adl+0],tep[0+8]); x1 = vec_xor(cfr[adl+1],tep[1+8]);		\
  x2 = vec_xor(cfr[adl+2],tep[2+8]); x3 = vec_xor(cfr[adl+3],tep[3+8]);		\
  x4 = vec_xor(cfr[adl+4],tep[4+8]); x5 = vec_xor(cfr[adl+5],tep[5+8]);		\
  x6 = vec_xor(cfr[adl+6],tep[6+8]); x7 = vec_xor(cfr[adl+7],tep[7+8]);		\
  csc_transP( 												\
  		  vec_xor(x7,(y7   =      vec_xor(cfr[adr+7],tep[7]))),			\
	      vec_xor(x6,(xy56 = vec_xor(x5,vec_xor(cfr[adr+6],tep[6])))),			\
	      vec_xor(x5,(y5   =      vec_xor(cfr[adr+5],tep[5]))),			\
	      vec_xor(x4,(xy34 = vec_xor(x3,vec_xor(cfr[adr+4],tep[4])))),			\
	      vec_xor(x3,(y3   =      vec_xor(cfr[adr+3],tep[3]))),			\
	      vec_xor(x2,(xy12 = vec_xor(x1,vec_xor(cfr[adr+2],tep[2])))),			\
	      vec_xor(x1,(y1   =      vec_xor(cfr[adr+1],tep[1]))),			\
	      vec_xor(x0,(xy70 = vec_xor(x7,vec_xor(cfr[adr+0],tep[0])))),			\
	  cfr[adl+7], cfr[adl+6], cfr[adl+5], cfr[adl+4],		\
	  cfr[adl+3], cfr[adl+2], cfr[adl+1], cfr[adl+0] );		\
  csc_transP( 											\
  		  vec_xor(x6,y7), xy56, vec_xor(x4,y5), xy34,					\
   	      vec_xor(x2,y3), xy12, vec_xor(x0,y1), xy70,					\
	  cfr[adr+7], cfr[adr+6], cfr[adr+5], cfr[adr+4],		\
	  cfr[adr+3], cfr[adr+2], cfr[adr+1], cfr[adr+0] );		\
  tep += 16;


  // global initializations
  memcpy( &(*subkey)[0], &key[1], sizeof((*subkey)[0]) );
  memcpy( &(*subkey)[1], &key[0], sizeof((*subkey)[1]) );
  int hs = 0;

  // cache initialization
  (*subkey)[2][56].vec = (*subkey)[2][48].vec = 
  (*subkey)[2][40].vec = (*subkey)[2][32].vec = (*subkey)[2][24].vec = vector_data_u1(_0);
  (*subkey)[2][16].vec = (*subkey)[2][ 8].vec = (*subkey)[2][ 0].vec = vector_data_u1(_1);
  tcp = &csc_tabc[0][8];
  skp = &(*subkey)[2][1].vec;
  skp1 = &(*subkey)[1][8].vec;
  {
  for( int n=7; n; n--,tcp+=8,skp1+=8,skp++ )
    csc_transP( 
        vec_xor(skp1[7],tcp[7]), vec_xor(skp1[6],tcp[6]), vec_xor(skp1[5],tcp[5]), vec_xor(skp1[4],tcp[4]),
	    vec_xor(skp1[3],tcp[3]), vec_xor(skp1[2],tcp[2]), vec_xor(skp1[1],tcp[1]), vec_xor(skp1[0],tcp[0]),
	    skp[56], skp[48], skp[40], skp[32], skp[24], skp[16], skp[ 8], skp[ 0] );
  }

  // bit 0 : average of 11.41 bits to twiddle
  // bit 1 : average of 11.12 bits to twiddle
  // bit 2 : average of 11.41 bits to twiddle
  // bit 3 : average of 12.16 bits to twiddle
  // bit 4 : average of 12.44 bits to twiddle
  // bit 5 : average of 12.06 bits to twiddle
  // bit 6 : average of 10.00 bits to twiddle
  // bit 7 : average of 10.94 bits to twiddle

  for( int i=2; i<8; i++ ) {
    u8 x = keyB[7-i] ^ csc_tabp[7-i]; x = csc_tabp[x] ^ csc_tabp[x ^ (1<<6)];
    int ntt = 0;
    (*totwiddle)[i-2][ntt++] = &(*subkey)[1][i*8+6].vec;
    for( int j=0; j<8; j++ ) {
      if( x & (1<<j) ) {
	unsigned n = j*8+i;
	(*totwiddle)[i-2][ntt++] = &(*subkey)[2][n].vec;
	(*totwiddle)[i-2][ntt++] = &(*tp1)[n/16][n & 7];
	if( (n & 15) <= 7 )
	  (*totwiddle)[i-2][ntt++] = &(*tp2)[n/16][n & 15];
	else {
	  (*totwiddle)[i-2][ntt++] = &(*tp2)[n/16][(n+1) & 7];
	  if( n & 1 )
	    (*totwiddle)[i-2][ntt++] = &(*tp1)[n/16][(n+1) & 7];
	}
      }
    }
    (*totwiddle)[i-2][ntt] = NULL;
  }

  skp = &(*subkey)[2][0].vec;
  {
  for( int n=0; n<4; n++ ) {
    x0 = vec_xor(msg[n*16+8+0],skp[0+8]); x1 = vec_xor(msg[n*16+8+1],skp[1+8]);
    x2 = vec_xor(msg[n*16+8+2],skp[2+8]); x3 = vec_xor(msg[n*16+8+3],skp[3+8]);
    x4 = vec_xor(msg[n*16+8+4],skp[4+8]); x5 = vec_xor(msg[n*16+8+5],skp[5+8]);
    x6 = vec_xor(msg[n*16+8+6],skp[6+8]); x7 = vec_xor(msg[n*16+8+7],skp[7+8]);

    (*tp1)[n][7] = vec_xor(x7,(y7 = vec_xor(msg[n*16+7],skp[7])));
    (*tp1)[n][6] = vec_xor(x6,((*tp2)[n][6] = vec_xor(x5,vec_xor(msg[n*16+6],skp[6]))));
    (*tp1)[n][5] = vec_xor(x5,(y5 = vec_xor(msg[n*16+5],skp[5])));
    (*tp1)[n][4] = vec_xor(x4,((*tp2)[n][4] = vec_xor(x3,vec_xor(msg[n*16+4],skp[4]))));
    (*tp1)[n][3] = vec_xor(x3,(y3 = vec_xor(msg[n*16+3],skp[3])));
    (*tp1)[n][2] = vec_xor(x2,((*tp2)[n][2] = vec_xor(x1,vec_xor(msg[n*16+2],skp[2]))));
    (*tp1)[n][1] = vec_xor(x1,(y1 = vec_xor(msg[n*16+1],skp[1])));
    (*tp1)[n][0] = vec_xor(x0,((*tp2)[n][0] = vec_xor(x7,vec_xor(msg[n*16+0],skp[0]))));

    (*tp2)[n][7] = vec_xor(x6,y7);
    (*tp2)[n][5] = vec_xor(x4,y5);
    (*tp2)[n][3] = vec_xor(x2,y3);
    (*tp2)[n][1] = vec_xor(x0,y1);

    skp += 16;
  }
  }

  for( ;; ) {

    //extern void printkey( ulong key[64], int n, bool tab );
    //printkey( subkey[1], 17, 1 );

    // local initializations
    memcpy( cfr, msg, sizeof(cfr) );

    // ROUND 1
    APPLY_MP0(  0,  8);
    APPLY_MP0( 16, 24);
    APPLY_MP0( 32, 40);
    APPLY_MP0( 48, 56);

    tep = &csc_tabe[0][0];
    APPLY_Me(  0, 16);
    APPLY_Me( 32, 48);
    APPLY_Me(  8, 24);
    APPLY_Me( 40, 56);
    APPLY_Me(  0, 32);
    APPLY_Me(  8, 40);
    APPLY_Me( 16, 48);
    APPLY_Me( 24, 56);

    // ROUNDS 2..8
    skp = &(*subkey)[3][0].vec;
    skp1 = &(*subkey)[2][0].vec;
    tcp = &csc_tabc[1][0];
    for( int sk=7; sk; sk-- ) {
      for( int n=8; n; n--,tcp+=8,skp1+=8,skp++ )
	csc_transP( 
	    vec_xor(skp1[7],tcp[7]), vec_xor(skp1[6],tcp[6]), vec_xor(skp1[5],tcp[5]), vec_xor(skp1[4],tcp[4]),
		vec_xor(skp1[3],tcp[3]), vec_xor(skp1[2],tcp[2]), vec_xor(skp1[1],tcp[1]), vec_xor(skp1[0],tcp[0]),
		skp[56], skp[48], skp[40], skp[32], skp[24], skp[16], skp[ 8], skp[ 0] );
      skp -= 8;

      APPLY_Ms(  0,  8);
      APPLY_Ms( 16, 24);
      APPLY_Ms( 32, 40);
      APPLY_Ms( 48, 56);

      tep = &csc_tabe[0][0];
      APPLY_Me(  0, 16);
      APPLY_Me( 32, 48);
      APPLY_Me(  8, 24);
      APPLY_Me( 40, 56);
      APPLY_Me(  0, 32);
      APPLY_Me(  8, 40);
      APPLY_Me( 16, 48);
      APPLY_Me( 24, 56);
    }

    // ROUND 9
    vulong result = vector_data_u4(_1,_1,_1,_1);
	vulong temp;
    {
    for( int n=0; n<8; n++,tcp+=8,skp1+=8,skp++ ) {
      csc_transP( vec_xor(skp1[7],tcp[7]), vec_xor(skp1[6],tcp[6]), vec_xor(skp1[5],tcp[5]), vec_xor(skp1[4],tcp[4]),
	      vec_xor(skp1[3],tcp[3]), vec_xor(skp1[2],tcp[2]), vec_xor(skp1[1],tcp[1]), vec_xor(skp1[0],tcp[0]),
	      skp[56], skp[48], skp[40], skp[32], skp[24], skp[16], skp[ 8], skp[ 0] );
    temp = (vec_xor(cipher[56+n],vec_xor(cfr[56+n],vec_xor(skp[56],skp[56-128]))));
    result = vec_and(result,vec_nor(temp,temp)); if( vec_all_eq(result,(vulong)(0)) ) goto stepper;
    temp = (vec_xor(cipher[48+n],vec_xor(cfr[56+n],vec_xor(skp[48],skp[48-128]))));
    result = vec_and(result,vec_nor(temp,temp)); if( vec_all_eq(result,(vulong)(0)) ) goto stepper;
    temp = (vec_xor(cipher[40+n],vec_xor(cfr[56+n],vec_xor(skp[40],skp[40-128]))));
    result = vec_and(result,vec_nor(temp,temp)); if( vec_all_eq(result,(vulong)(0)) ) goto stepper;
    temp = (vec_xor(cipher[32+n],vec_xor(cfr[56+n],vec_xor(skp[32],skp[32-128]))));
    result = vec_and(result,vec_nor(temp,temp)); if( vec_all_eq(result,(vulong)(0)) ) goto stepper;
    temp = (vec_xor(cipher[24+n],vec_xor(cfr[56+n],vec_xor(skp[24],skp[24-128]))));
    result = vec_and(result,vec_nor(temp,temp)); if( vec_all_eq(result,(vulong)(0)) ) goto stepper;
    temp = (vec_xor(cipher[16+n],vec_xor(cfr[56+n],vec_xor(skp[16],skp[16-128]))));
    result = vec_and(result,vec_nor(temp,temp)); if( vec_all_eq(result,(vulong)(0)) ) goto stepper;
    temp = (vec_xor(cipher[ 8+n],vec_xor(cfr[56+n],vec_xor(skp[ 8],skp[ 8-128]))));
    result = vec_and(result,vec_nor(temp,temp)); if( vec_all_eq(result,(vulong)(0)) ) goto stepper;
    temp = (vec_xor(cipher[ 0+n],vec_xor(cfr[56+n],vec_xor(skp[ 0],skp[ 0-128]))));
    result = vec_and(result,vec_nor(temp,temp)); if( vec_all_eq(result,(vulong)(0)) ) goto stepper;
    }
    }
    memcpy( &key[0], &(*subkey)[1], sizeof(key[0]) );
  //memcpy( &key[1], &(*subkey)[0], sizeof(key[1]) );
    return result;

  stepper:
    // increment the key in gray order
    hs++;
    // bits 6
    if( hs & (1 << 0) ) { //(*subkey)[1][22] = ~subkey[1][22];
      for( vulong **p = &(*totwiddle)[0][0]; *p; p++ ) **p = vec_xor(**p,(vulong)(_1));
      continue;
    }
    if( hs & (1 << 1) ) { //(*subkey)[1][30] = ~(*subkey)[1][30];
      for( vulong **p = &(*totwiddle)[1][0]; *p; p++ ) **p = vec_xor(**p,(vulong)(_1));
      continue;
    }
    if( hs & (1 << 2) ) { //(*subkey)[1][38] = ~(*subkey)[1][38];
      for( vulong **p = &(*totwiddle)[2][0]; *p; p++ ) **p = vec_xor(**p,(vulong)(_1));
      continue;
    }
    if( hs & (1 << 3) ) { //(*subkey)[1][46] = ~(*subkey)[1][46];
      for( vulong **p = &(*totwiddle)[3][0]; *p; p++ ) **p = vec_xor(**p,(vulong)(_1));
      continue;
    }
    if( hs & (1 << 4) ) { //(*subkey)[1][54] = ~(*subkey)[1][54];
      for( vulong **p = &(*totwiddle)[4][0]; *p; p++ ) **p = vec_xor(**p,(vulong)(_1));
      continue;
    }
    if( hs & (1 << 5) ) { //(*subkey)[1][62] = ~(*subkey)[1][62];
      for( vulong **p = &(*totwiddle)[5][0]; *p; p++ ) **p = vec_xor(**p,(vulong)(_1));
      continue;
    }
    break;
  }

  return (vulong)(0);
}