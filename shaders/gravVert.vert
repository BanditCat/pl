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
layout(binding = 1) uniform sampler2D texSampler;
layout( binding = 2) buffer colorBuffer{
   vec3 colors[];
};
struct particle{
  vec3 pos;
  vec3 vel;
  float color;
  float mass;
};
layout( binding = 3) buffer particleBuffer{
  particle ps[];
};
layout( binding = 4 ) buffer projBuffer{
  mat4 proj;
};

layout(location = 0) out vec2 position;
layout(location = 1) flat out vec3 color;

const vec2 positions[ 3 ] = vec2[](
				   vec2( -0.5, -0.28867513459 ), 
				   vec2( 0.5, -0.28867513459 ),
				   vec2( 0, 0.57735026919 )
				   );

void main(){
  ivec2 size = textureSize( texSampler, 0 );
  int tindex =  gl_VertexIndex / 3;
  int pindex = gl_VertexIndex % 3;
  position = positions[ int( pindex ) ] * 3.46410161514;
  vec4 ppos = vec4( ps[ tindex ].pos, 1.0 ) + vec4( position * 0.0412, 0, 0 );
  vec4 spos = (proj* ppos);// / ppos.w;
  gl_Position = 
       spos;
  color = colors[ tindex ];
}

