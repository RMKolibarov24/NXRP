#version 120
uniform sampler2D surface;
uniform bool textured;
uniform float darkness;
uniform float flicker;
uniform float flash;
uniform float wetness;
varying vec3 eyePosition;
varying vec3 eyeNormal;
varying vec4 tint;
void main()
{
    vec3 normal = normalize(eyeNormal);
    float distanceToEye = max(length(eyePosition), 0.001);
    vec3 towardLight = -eyePosition / distanceToEye;
    float diffuse = max(dot(normal, towardLight), 0.0);
    float cone = smoothstep(0.60 + darkness * 0.07, 0.90, dot(-towardLight, vec3(0.0, 0.0, -1.0)));
    float attenuation = 1.0 / (1.0 + distanceToEye * 0.10 + distanceToEye * distanceToEye * 0.035);
    float moon = max(dot(normal, normalize(gl_LightSource[1].position.xyz)), 0.0);
    vec3 ambient = vec3(0.12, 0.15, 0.14) * (1.0 - darkness * 0.45);
    vec3 moonlight = vec3(0.17, 0.24, 0.28) * moon * 0.65;
    vec3 stormlight = vec3(0.34, 0.40, 0.52) * flash * (0.45 + moon * 0.55);
    vec3 flashlight = vec3(1.15, 0.92, 0.62) * diffuse * cone * attenuation * flicker;
    vec3 shelterOffset = gl_LightSource[2].position.xyz - eyePosition;
    float shelterDistance = max(length(shelterOffset), 0.001);
    float shelterDiffuse = max(dot(normal, shelterOffset / shelterDistance), 0.0);
    vec3 shelter = gl_LightSource[2].diffuse.rgb * (0.20 + shelterDiffuse * 0.80)
        / (1.0 + shelterDistance * 0.28 + shelterDistance * shelterDistance * 0.13);
    vec4 texel = textured ? texture2D(surface, gl_TexCoord[0].xy) : vec4(1.0);
    vec3 color = texel.rgb * tint.rgb * (ambient + moonlight + stormlight + flashlight + shelter);
    float sheen = pow(max(dot(normal, towardLight), 0.0), 28.0) * cone * attenuation * flicker * wetness;
    color += vec3(0.70, 0.61, 0.47) * sheen;
    float fogAmount = 1.0 - exp(-pow(gl_Fog.density * distanceToEye, 2.0));
    gl_FragColor = vec4(mix(color, gl_Fog.color.rgb, clamp(fogAmount, 0.0, 1.0)), texel.a * tint.a);
}
