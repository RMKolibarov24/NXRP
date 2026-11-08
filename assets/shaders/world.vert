#version 120
varying vec3 eyePosition;
varying vec3 eyeNormal;
varying vec4 tint;
void main()
{
    vec4 position = gl_ModelViewMatrix * gl_Vertex;
    eyePosition = position.xyz;
    eyeNormal = normalize(gl_NormalMatrix * gl_Normal);
    tint = gl_Color;
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_Position = gl_ProjectionMatrix * position;
}
