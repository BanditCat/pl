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

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec2 pos;
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main(){ 
   outColor = texture(texSampler, fragTexCoord) + vec4( 0.0, 0.05, 0.0, 0.0 );
}
 
