#version 120
uniform sampler2D surface;
uniform float scroll;
uniform vec2 nextCell;
varying vec2 uv;
varying vec3 shade;
void main()
{
    // Carry the next source symbol into the window rather than snapping tiles.
    vec2 cell=floor(uv*5.0);
    vec2 local=fract(uv*5.0);
    local.y+=scroll;
    if(local.y>=1.0) { local.y-=1.0; cell=nextCell; }
    local=clamp(local,vec2(.008),vec2(.992));
    gl_FragColor=vec4(texture2D(surface,(cell+local)/5.0).rgb*shade,1.0);
}
