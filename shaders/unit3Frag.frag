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

layout(location = 0) out vec4 outColor;

void main(){
  vec2 c = pos;
  vec2 x = c + fragTexCoord;
  for( int i = 0; i < 2000; ++i ){
    vec2 t = x;
    x = vec2( t.x * t.x - t.y * t.y, 2 * t.x * t.y );
    x += c;
  }
  float cl = dot( x, x );
  outColor = vec4( cl, cl, cl, 1.0 );
}
 
