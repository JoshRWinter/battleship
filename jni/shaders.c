const char *vertexshader=
"attribute vec2 pos;"
"uniform mat4 projection;"
"uniform vec4 texcoords;"
"uniform vec2 vector,size;"
"uniform float rot;"
"varying vec2 ftexcoord;"
"void main(){"
"mat4 model=mat4(1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0,vector.x+(size.x/2.0),vector.y+(size.y/2.0),0.0,1.0);"
"mat4 rotation=mat4(cos(rot),-sin(rot),0.0,0.0,sin(rot),cos(rot),0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0);"
"gl_Position=projection*model*rotation*vec4(pos.x*size.x,pos.y*size.y,0.0,1.0);"
"if(pos.x<0.0&&pos.y<0.0)ftexcoord=vec2(texcoords[0],texcoords[3]);"
"else if(pos.x<0.0&&pos.y>0.0)ftexcoord=vec2(texcoords[0],texcoords[2]);"
"else if(pos.x>0.0&&pos.y<0.0)ftexcoord=vec2(texcoords[1],texcoords[3]);"
"else ftexcoord=vec2(texcoords[1],texcoords[2]);"
"}"
,*fragmentshader=
"varying lowp vec2 ftexcoord;"
"uniform lowp vec4 rgba;"
"uniform sampler2D tex;"
"void main(){"
"gl_FragColor=texture2D(tex,ftexcoord)*rgba;"
"}"
;
