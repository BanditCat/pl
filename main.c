////////////////////////////////////////////////////////////////////////////////
//     Copyright (c) Jon DuBois 2022. This file is part of pseudoluminal.     //
//                                                                            //
// This program is free software: you can redistribute it and/or modify       //
// it under the terms of the GNU Affero General Public License as published   //
// by the Free Software Foundation, either version 3 of the License, or       //
// (at your option) any later version.                                        //
//                                                                            //
// This program is distributed in the hope that it will be useful,            //
// but WITHOUT ANY WARRANTY; without even the implied warranty of             //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              //
// GNU Affero General Public License for more details.                        //
//                                                                            //
// You should have received a copy of the GNU Affero General Public License   //
// along with this program.  If not, see <https://www.gnu.org/licenses/>.     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
// Main entry point.                                                          //
////////////////////////////////////////////////////////////////////////////////

#include <windows.h>

#include "pl.h"
#include "prog.h"
#include "util.h"
#include "os.h"
#include "gui.h"
#include "vk.h"
#include "input.h"


typedef struct asp{
  f32 mulx;
  f32 muly;
} asp;
  
void afunc( void* v, void* mem ){
  asp* a = (asp*)v;
  plvkUnit** u = (plvkUnit**)mem;
  if( *u ){
    u64 w, h;
    plvkGetUnitSize( *u, &w, &h );
    a->mulx = fsqrt( ( (f32)h ) / ( (f32)w ) );
    a->muly = 1 / a->mulx;
  }
}

  
const char* clUsage =
  ////////////////////////////////////////
  ////////////////////////////////////////
  "Command line options:\n"
  "\n"
  "-window=<H>x<W>,<X>,<Y>    Set the size"
  " size of the window to H height at W \n"
  "                           width at the"
  " screen position specified by X and Y\n"
  "\n"
  "-gpu=<G>                   Use the gpu "
  "specified by G, which is an integer.\n"
  "\n"
  "-info                      List the gpu"
  "s and other information.\n"
  "\n"
  "-fps                       Print fps to "
  "stdout every second.\n"
  "\n"
  "-compressorOutput=name     The output fi"
  "lename for the compressor.\n"
  "\n"
  "-compressDir=dir           Compress the "
  "indicated directory.\n"
  "\n"
#ifdef DEBUG  
  "-debugLevel=<D>            Prevents val"
  "idation messages with severity less than\n"
  "                           D from being"
  " output. (default is 2).\n";
#else
;
#endif

// Default window position
#define defX 100
#define defY 100
#define minW 320
#define minH 240

int main( int argc, const char** argv ){
  marc;
  u32 x = defX;
  u32 y = defY;
  u32 w = minW;
  u32 h = minH;
  // -1 means pick GPU based on score.
  int gpu = -1;
  u32 debugLevel = 2;

  // For compression.
  const char* compressorOutput = NULL;
  const char* compressDir = NULL;
  
  // Parse and execute command line.
  for( int i = 1; i < argc; ++i ){
    if( strStartsWith( "-window=", argv[ i ] ) ){
      const char* opt = argv[ i ] + slen( "-window=" );
      const char* trackOpt = opt;
      w = parseInt( &opt );
      if( opt == trackOpt || *opt != 'x' )
	die( "Malformed -window= command line option." );
      ++opt;
      trackOpt = opt;
      h = parseInt( &opt );
      if( opt == trackOpt || *opt != ',' )
	die( "Malformed -window= command line option." );
      ++opt;
      trackOpt = opt;
      x = parseInt( &opt );
      if( opt == trackOpt || *opt != ',' )
	die( "Malformed -window= command line option." );
      ++opt;
      trackOpt = opt;
      y = parseInt( &opt );
      if( opt == trackOpt || *opt )
	die( "Malformed -window= command line option." );
      continue;
    }
    if( strStartsWith( "-gpu=", argv[ i ] ) ){
      const char* opt = argv[ i ] + slen( "-gpu=" );
      const char* trackOpt = opt;
      gpu = parseInt( &opt );
      if( opt == trackOpt || *opt )
	die( "Malformed -gpu= command line option." );
      continue;
    }
    if( strStartsWith( "-compressorOutput=", argv[ i ] ) ){
      const char* opt = argv[ i ] + slen( "-compressorOutput=" );
      if( !slen( opt ) )
	die( "Malformed -compressorOutput= command line option." );
      compressorOutput = opt;
      continue;
    }
    if( strStartsWith( "-compressDir=", argv[ i ] ) ){
      const char* opt = argv[ i ] + slen( "-compressDir=" );
      if( !slen( opt ) )
	die( "Malformed -compressDir= command line option." );
      compressDir = opt;
      continue;
    }
#ifdef DEBUG    
    if( strStartsWith( "-debugLevel=", argv[ i ] ) ){
      const char* opt = argv[ i ] + slen( "-debugLevel=" );
      const char* trackOpt = opt;
      debugLevel = parseInt( &opt );
      if( opt == trackOpt || *opt )
	die( "Malformed -debugLevel= command line option." );
      continue;
    }
#endif
    if( !strcomp( "-fps", argv[ i ] ) ){
      state.fps = 1;
      continue;
    }
    if( !strcomp( "-info", argv[ i ] ) ){
      plvkInit( gpu, debugLevel, true );
      plvkPrintInitInfo();
      plvkPrintGPUs();
      return 0;
    }
    if( !strcomp( argv[ i ], "-help" ) || !strcomp( argv[ i ], "--help" ) ||
	!strcomp( argv[ i ], "-?" ) || !strcomp( argv[ i ], "/?" ) ){
      printl( clUsage );
      return 0;
    }
    print( "Unrecognized command line option " );
    printl( argv[ i ] );
    printl( clUsage );
    return 1;
  }
  if( compressDir && !compressorOutput ){
    die( "Directory to compress given, but no output specified." );
  }else if( !compressDir && compressorOutput ){
    die( "Compression output file given, but no directory specified." );
  }else if( compressDir && compressorOutput ){
    print( "Compressing directory " ); print( compressDir );
    print( " to file " ); printl( compressorOutput );
    hasht* ht = htLoadDirectory( compressDir );
    htPrint( ht );
    u64 ssize;
    const char* sd = htSerialize( ht, &ssize );
    htDestroy( ht );
    u64 csize;
    const char* cd = compressOrDie( sd, ssize, &csize );
    memfree( (void*)sd );
    print( "Original size:   " ); printInt( ssize ); endl();
    print( "Compressed size: " ); printInt( csize ); endl();
    print( "Difference:      " ); printInt( ssize - csize ); endl();
    print( "Ratio:           " ); printFloat( (f64)( csize ) / (f64)ssize );
    endl();
    writeFileOrDie( compressorOutput, cd, csize );
    memfree( (void*)cd );
    return 0;
  }
  // Get resources.
  getRes();
  marc;
  // Initialize vulkan.
#ifdef DEBUG
  printl( "Initializing vulkan..." );
#endif
  marc;

  plvkInstance* vk = plvkInit( gpu, debugLevel, true );
  marc;
  plvkAttachable* atts[] = { plvkAddTexture( vk, "graphics\\tp.ppm" ),
    plvkAddTexture( vk, "graphics\\greekλLambda.ppm" ),
    plvkAddTexture( vk, "graphics\\lc.ppm" ), NULL, NULL, NULL, NULL, NULL };
  plvkUnit* u1 =plvkCreateUnit( vk, 640, 400, VK_FORMAT_R8G8B8A8_UNORM, 4,
				 "shaders\\unitFrag.spv",
				 "shaders\\quad.spv",
				 true, "foo", 300, 300, atts, 1, 6, NULL, 1, NULL, 0 );
  marc;
  static const u32 gsz = 192;
 
    static const u32 cuesz = 8;
    static const u32 cusz = gsz * gsz;
    seedRand( 31337 );
    newa( ps, f32, gsz * gsz * 4 );
    newa( cud, f32, cusz * cuesz * 4 );
    for( u32 x = 0; x < gsz; ++x ){
      for( u32 y = 0; y < gsz; ++y ){
	ps[ ( y * gsz + x ) * 4 + 0 ] = frand( -1.0, 1.0 );
	ps[ ( y * gsz + x ) * 4 + 1 ] = frand( -1.0, 1.0 );
	ps[ ( y * gsz + x ) * 4 + 2 ] = frand( -0.00001, 0.00001 );
	ps[ ( y * gsz + x ) * 4 + 3 ] = frand( -0.00001, 0.00001 );
	for( int i = 0; i < 3; ++i )
	  cud[ ( y * gsz + x ) * cuesz + i ] = frand( -1.0, 1.0 );
	for( int i = 4; i < 7; ++i )
	  cud[ ( y * gsz + x ) * cuesz + i ] = frand( -0.000001, 0.000001 );
	cud[ ( y * gsz + x ) * cuesz + 3 ] = frand( 0.5, 1.0 );
	cud[ ( y * gsz + x ) * cuesz + 7 ] = frand( 0.5, 2.5 );
      }

    }
    marc;
    plvkCreateUnit( vk, gsz, gsz, VK_FORMAT_R32G32B32A32_SFLOAT, 16,
		    "shaders\\gravFrag.spv",
		    "shaders\\quad.spv",
		    false, "foo", 400, 400, NULL, 0, 6, (u8*)ps,0, NULL, 0  );
    memfree( ps );
    ps = newae( f32, gsz * gsz * 4 );
    for( u32 x = 0; x < gsz; ++x ){
      for( u32 y = 0; y < gsz; ++y ){
	ps[ ( y * gsz + x ) * 4 + 0 ] = frand( 0.2, 1.0 );
	ps[ ( y * gsz + x ) * 4 + 1 ] = frand( 0.5, 1.0 );
	ps[ ( y * gsz + x ) * 4 + 2 ] = frand( 0.3, 1.0 );
	ps[ ( y * gsz + x ) * 4 + 3 ] = frand( 0.5, 1.0 );
      }
    }
    marc;
    plvkAddBuffer( vk, ps, gsz * gsz * 4 * 4 );
    marc;
    const u32 tileSize = 192;
    plvkUnit* gc = plvkCreateUnit( vk, cusz / tileSize, 1, 0, cuesz * 4,
				   "shaders\\gravcomp.spv", NULL,
				   false, "foofff", 50, 200, NULL, 0,
				   cusz, cud, 1, NULL, 0  );
    (void)gc;
    /* for( u32 z = 0; z < 1; z++ ){ */
    /*   plvkTickUnit( gc ); */
    /*   f32* gd = plvkCopyComputeBuffer( gc ); */
    /*   for( u32 y = 0; y < 100; y++ ){ */
    /* 	endl(); */
    /* 	  print( " " ); printFloat( gd[ y ] ); */
    /*   } */
    /*   memfree( gd ); */
    /*   endl(); */
    //    }
    marc;
    memfree( ps );
    memfree( cud );

  
  marc;
  atts[ 3 ] = plvkGetAttachable( vk, 2 );
  atts[ 4 ] = plvkGetAttachable( vk, 1 );
  atts[ 5 ] = plvkGetAttachable( vk, 0 );
  plvkUnit* u2 =  plvkCreateUnit( vk, 640, 400, VK_FORMAT_R8G8B8A8_UNORM, 4,
				  "shaders\\unitFrag.spv",
				  "shaders\\quad.spv",
				  true, "foo", 400, 400, atts + 1, 1, 6, NULL,
				  1, NULL, 0  );
  marc;
  plvkUnit* u3 =  plvkCreateUnit( vk, 1000, 1000, VK_FORMAT_R8G8B8A8_UNORM, 4,
				  "shaders\\unit2Frag.spv",
				  "shaders\\quad.spv",
				  true, "foo", 1050, 200, atts + 3, 1, 6, NULL,
				  1, NULL, 0  );
  marc;
  new( u4, plvkUnit* );
  plvkAddUniformBuffer( vk, sizeof( asp ), afunc, u4 );
  marc;
  atts[ 7 ] = plvkGetAttachable( vk, 0 );
  marc;
  plvkCreateUnit( vk, 1, 1, 0, 4, "shaders\\moveComp.spv", NULL, false,
		  NULL, 0, 0, atts + 7, 1, 256, NULL, 1, NULL, 0 );
  atts[ 6 ] = plvkGetAttachable( vk, 0 );
  marc;
  *u4 = plvkCreateUnit( vk, 1000, 1000, VK_FORMAT_R8G8B8A8_UNORM, 4,
			"shaders\\unit3Frag.spv",
			"shaders\\gravVert.spv",
			true, "foofff", 50, 200, atts + 4, 3,
			gsz * gsz * 3, NULL, 1, NULL, 0 );
  
  plvkShow( u1 );
  plvkShow( u2 );
  plvkShow( u3 );
  plvkShow( *u4 );


  marc;
  {
    u32 tdimx = 16;
    u32 tdimy = 16;
    u32 tsz = 256;
    newa( ta, f32, tsz );
    newa( tb, f32, tsz );
    newa( tc, f32, tsz );
    for( u32 x = 0; x < tdimx; x++ ){
      for( u32 y = 0; y < tdimy; y++ ){
	ta[ y * tdimx + x ] = x == y ? 1.0 : 0.0;
	tb[ y * tdimx + x ] = (y * tdimx + x) * 0.01;
	tc[ y * tdimx + x ] = 0.1;
      }
    }
    plvkAddBuffer( vk, tb, tsz * 4 );
    plvkAddBuffer( vk, tc, tsz * 4 );
    plvkAttachable* tatts[] = {
      plvkGetAttachable( vk, 1 ),
      plvkGetAttachable( vk, 0 ) };

    //tb[ 2 ] = 2;
    plvkUnit* testUnit = plvkCreateUnit( vk, 1, 1, 0, 4,
					 "shaders\\test.spv", NULL,
					 false, "tttt", 0, 0, tatts, 2,
					 tsz, (const void*)ta, 1, NULL, 0 );
    for( u32 y = 0; y < tdimy; y++ ){
      endl();
      for( u32 x = 0; x < tdimx; x++ ){
	print( " " ); printFloat( tb[ y * tdimx + x ] );
      }
    }
    for( u32 y = 0; y < tdimy; y++ ){
      endl();
      for( u32 x = 0; x < tdimx; x++ ){
	print( " " ); printFloat( tc[ y * tdimx + x ] );
      }
    }
    char* gd;
    for( u64 i = 0; i < 10; ++i ){
      printInt( i ); endl();
      gd = plvkCopyComputeBuffer( testUnit );
      f32* fd = (f32*)gd;
      for( u32 y = 0; y < tdimy; y++ ){
	endl();
	for( u32 x = 0; x < tdimx; x++ ){
	  print( " " ); printFloat( fd[ y * tdimx + x ] );
	}
      }
      plvkTickUnit( testUnit );
      memfree( gd );
      endl();
    }

    memfree( ta );
    memfree( tb );
    memfree( tc );
  }
					 
  // Main loop.
  plvkStartRendering( vk );
  bool done = false;
  while( !done && plvkeventLoop( vk ) ){
    for( u32 dev = 0; dev < htElementCount( state.osstate->devices ); ++dev ){
      inputDevice** devp =
	(inputDevice**)htByIndex( state.osstate->devices, dev, NULL );
    
      inputDevice* dev;
      if( devp ){
	dev = *devp;
	if( dev->type == GPU_OTHER && dev->buttons[ 7 ] )
	  done = true;

	if( dev->type == GPU_KEYBOARD && dev->buttons[ 27 ] )
	  done = true;
      }
    }
  }
  // Run tests.
#ifdef DEBUG
  testPrograms();
  htTest();
#endif
  memfree( u4 );
  return 0;
}

