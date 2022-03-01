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

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

vec2 positions[3] = vec2[](
			   vec2( 0.0, -0.8 ), 
			   vec2( 0.5, 0.3 ),
			   vec2( -0.5, 0.5 )
			   );

vec3 colors[3] = vec3[](
			vec3(1.0, 0.0, 0.0 ),
			vec3(0.0, 1.0, 0.0 ),
			vec3(0.0, 0.0, 1.0 )
			);

void main(){
  gl_Position = vec4( positions[ gl_VertexIndex ], 0.0, 1.0) +
    vec4( mod( ubo.time * 10, 2.0 ) - 1.0, 0.0, 0.0, 0.0 );
  fragColor = colors[ gl_VertexIndex ];
  fragTexCoord = vec2( gl_Position );
  
}

