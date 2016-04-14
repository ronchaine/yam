#version 330
precision highp float;

in vec4 ex_col;
in vec2 texcoord0;

uniform sampler2D texture;

void main()
{
   vec4 out_colour = texture2D(texture, texcoord0);
   gl_FragColor = out_colour;
}
