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
  ivec2 size = textureSize( texSampler, 0 );
  float countscale = 1.0 / ( size.x * size.y );
  ivec2 spos = ivec2( pos * size );
  vec4 self = texelFetch( texSampler, spos, 0 );
  vec2 selfPos = self.xy;
  vec2 selfVel = self.zw;
  vec2 force = vec2( 0, 0 );
  ivec2 tpos;
  for( tpos.x = 0; tpos.x < size.x; ++tpos.x ){
    for( tpos.y = 0; tpos.y < size.y; ++tpos.y ){
      if( spos != tpos ){
 
	vec2 other = texelFetch( texSampler, tpos, 0 ).xy;
	vec2 diff = other - selfPos;
	float d = sqrt( dot( diff, diff ) );
	d = clamp( d, 0.00155, 4 );
	force += diff / ( d * d * d );
      }
    }
    selfVel += force * 0.000005 * countscale;
    selfPos += selfVel * 0.0000058;
  }

  float veld = sqrt( dot( selfVel, selfVel ) );
  if( veld > 1 )
    selfVel /= veld;
  outColor = vec4( clamp( selfPos, -1.0, 1.0 ), selfVel );
  
}
 
