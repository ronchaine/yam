namespace yam
{
   static const char* ui_fragmentshader_glsl = 
   {
      "#version 140\n"
      "precision highp float;\n"
      "\n"
      "in vec2 texcoord0;\n"
      "in vec4 ex_col;\n"
      "\n"
      "uniform sampler2D guiatlas;\n"
      "\n"
      "void main()\n"
      "{\n"
      "   vec4 alpha = texture2D(guiatlas, texcoord0);\n"
      "   vec4 out_colour = vec4(ex_col.r,\n"
      "                          ex_col.g,\n"
      "                          ex_col.b,\n"
      "                          ex_col.a * alpha.r);\n"
      "\n"
      "   gl_FragColor = out_colour;\n"
      "}\n"
   };

   static const char* ui_vertexshader_glsl =
   {
      "#version 140\n"
      "#extension GL_ARB_explicit_attrib_location : require\n"
      "\n"
      "layout(location = 0) in vec2 in_pos;\n"
      "layout(location = 1) in vec2 in_tcd;\n"
      "layout(location = 2) in vec4 in_col;\n"
      "\n"
      "out vec2 texcoord0;\n"
      "out vec4 ex_col;\n"
      "\n"
      "uniform sampler2D guiatlas;\n"
      "\n"
      "void main()\n"
      "{\n"
      "   ex_col = in_col;\n"
      "   texcoord0 = in_tcd;\n"
      "\n"
      "   gl_Position = vec4(in_pos, 0.0, 1.0);\n"
      "}\n"
   };
}
