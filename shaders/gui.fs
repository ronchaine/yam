#version 140
precision highp float;

in vec2 texcoord0;
in vec4 ex_col;

uniform sampler2D guiatlas;

void main()
{
   vec4 alpha = texture2D(guiatlas, texcoord0);
   vec4 out_colour = vec4(ex_col.r,
                          ex_col.g,
                          ex_col.b,
                          ex_col.a * alpha.r);

   gl_FragColor = out_colour;
}
