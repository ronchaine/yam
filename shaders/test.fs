#version 140
precision highp float;

in vec4 ex_col;

void main()
{
   vec4 out_colour = vec4(ex_col);
   //gl_FragColor = vec4(out_colour.rgb, 1.0);
   gl_FragColor = out_colour;
}
