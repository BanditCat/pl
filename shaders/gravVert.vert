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
// Display fragment shader.                                                   //
////////////////////////////////////////////////////////////////////////////////

#version 460

layout(binding = 0) uniform UniformBufferObject {
  float time;
} ubo; 
layout( binding = 1) buffer colorBuffer{
   vec4 colors[];
};
struct particle{
  vec4 pos;
  vec4 vel;
};
layout( binding = 2) buffer particleBuffer{
  particle ps[];
};
layout( binding = 3 ) buffer projBuffer{
  mat4 projm;
  mat4 rotm;
  mat4 irotm;
  mat4 posm;
  vec3 pos;
  float fov;
  vec3 rot;
};

layout(location = 0) out vec2 position;
layout(location = 1) flat out vec3 color;

const vec2 positions[ 3 ] = vec2[](
				   vec2( -0.5, -0.28867513459 ), 
				   vec2( 0.5, -0.28867513459 ),
				   vec2( 0, 0.57735026919 )
				   );

void main(){
  int tindex =  gl_VertexIndex / 3;
  int pindex = gl_VertexIndex % 3;
  position = positions[ int( pindex ) ] * 3.46410161514;
  vec4 ppos = vec4( ps[ tindex ].pos.xyz, 1.0 ) ;
  vec4 spos = (projm * rotm * posm * ppos) + projm *
    vec4( position * 0.0112, 0, 1 );
  gl_Position = 
       spos;
  color = colors[ tindex ].rgb;
}

