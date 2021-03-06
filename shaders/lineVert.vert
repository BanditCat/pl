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
// Wireframe vertex shader.                                                   //
////////////////////////////////////////////////////////////////////////////////

#version 460
#include "interface.glib"
#include "funcs.glib"

struct line{
  vec4 begin;
  vec4 end;
};
layout( binding = 3 ) buffer linesBuffer{
  line points[];
};
layout( binding = 2 ) buffer projBuffer{
  mat4 projm;
  mat4 rotm;
  mat4 irotm;
  mat4 posm;
  vec3 pos;
  float fov;
  vec3 rot;
};

layout(location = 1) flat out vec3 color;

const bool which[ 6 ] = { false, true, false, false, true, true };

const vec2 positions[ 6 ] = vec2[](
				   vec2( 0, -1.0 ), 
				   vec2( 0, -1.0 ),
				   vec2( 0, 1.0 ),
				   vec2( 0, 1.0 ),
				   vec2( 0, -1.0 ), 
				   vec2( 0, 1.0 )
				   );

void main(){
  int tindex =  gl_VertexIndex / 6;
  int pindex = gl_VertexIndex % 6;
  float size = 1;
  vec3 lvec = ( points[ tindex ].begin.xyz - points[ tindex ].end.xyz );
  lvec = ( rotm * vec4( lvec, 1.0 ) ).xyz;
  vec3 position = normalize( cross( vec3( lvec.x, lvec.y, 0 ),
				    vec3( 0, 0, positions[ pindex ].y ) ) );
  vec3 lpos = points[ tindex ].begin.xyz;
  if( which[ pindex ] )
    lpos = points[ tindex ].end.xyz;
  vec4 ppos = rotm * posm * vec4( lpos, 1.0 );
  vec4 spos = ppos + //projm *
//    vec4( ( faceMat( ppos.xyz ) *
	    vec4( position * 0.0152 * size, 1 );// );//.xyz, 1 );
  // vec4 spos = ppos + //projm *
  //   vec4( ( faceMat( lpos.xyz ) *
  // 	    vec4( position * 0.0152 * size, 1 ) ).xyz, 1 );
  gl_Position = 
    projm * spos;
  color = hsv2rgb( vec3( points[ tindex ].end.w, 0.5, 1.0 ) );
}

