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

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec2 pos;

const vec2 positions[ 4 ] = vec2[](
				   vec2( -1.0, -1.0 ), 
				   vec2( 1.0, -1.0 ),
				   vec2( -1.0, 1.0 ),
				   vec2( 1.0, 1.0 )
				   );

const uint indices[ 6 ] = { 0, 1, 2, 2, 1, 3 };

void main(){
  gl_Position = vec4( positions[ indices[ gl_VertexIndex ] ], 0.0, 1.0); 
  pos = ( gl_Position.xy + vec2( 1.0, 1.0 ) ) * vec2( 0.5, 0.5 );
  
  fragTexCoord = vec2( vec4( mod( ubo.time , 2.0 ) - 1.0, 0.0, 0.0, 0.0 ) + gl_Position );
  
}

