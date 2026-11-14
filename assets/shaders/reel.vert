#version 120
varying vec2 uv;
varying vec3 shade;
void main()
{
    vec3 eye=(gl_ModelViewMatrix*gl_Vertex).xyz;
    vec3 normal=normalize(gl_NormalMatrix*gl_Normal);
    vec3 light=normalize(gl_LightSource[2].position.xyz-eye);
    shade=gl_Color.rgb*(vec3(.53,.42,.32)+vec3(.38,.32,.23)*max(dot(normal,light),0.0));
    uv=gl_MultiTexCoord0.xy;
    gl_Position=ftransform();
}
