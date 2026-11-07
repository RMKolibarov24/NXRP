#version 120

// Screen-space ray-traced lighting pass: ray-marched reflections, contact
// ambient occlusion and travelling light through the fog. It is deliberately
// screen-space so the fixed-pipeline renderer stays portable.
uniform sampler2D scene;
uniform sampler2D depthTex;
uniform vec2 resolution;
uniform float time;
uniform float fear;
uniform float threat;
uniform float flash;
uniform float darkness;
uniform float rayTrace;
uniform float nearPlane;
uniform float farPlane;
uniform float tanHalfFov;
uniform float aspect;
uniform float seed;
varying vec2 uv;

float hash21(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21) + seed * .013);
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float linearDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

vec3 viewPoint(vec2 texUv, float distanceToEye)
{
    vec2 ndc = texUv * 2.0 - 1.0;
    return vec3(ndc.x * tanHalfFov * aspect, ndc.y * tanHalfFov, -1.0) * distanceToEye;
}

vec2 projectView(vec3 point)
{
    float z = max(-point.z, 0.001);
    vec2 ndc = vec2(point.x / (z * tanHalfFov * aspect), point.y / (z * tanHalfFov));
    return ndc * 0.5 + 0.5;
}

vec3 surfaceNormal(vec3 point)
{
    vec3 dx = dFdx(point);
    vec3 dy = dFdy(point);
    vec3 normal = cross(dx, dy);
    float lengthSquared = dot(normal, normal);
    if (lengthSquared < 0.000001) return vec3(0.0, 0.0, 1.0);
    normal /= sqrt(lengthSquared);
    if (dot(normal, -normalize(point)) < 0.0) normal = -normal;
    return normal;
}

float ambientOcclusion(vec2 texUv, vec3 point, vec3 normal, float distanceToEye)
{
    float visible = 1.0;
    for (int i = 0; i < 6; ++i)
    {
        float angle = float(i) * 2.39996323 + seed * .017;
        float radius = (float(i) + 1.0) / 6.0;
        vec2 offset = vec2(cos(angle), sin(angle)) * (5.0 + 25.0 * radius) / resolution;
        vec2 sampleUv = texUv + offset;
        float sampleDepth = linearDepth(texture2D(depthTex, sampleUv).r);
        if (sampleDepth >= farPlane * 0.985) continue;
        vec3 samplePoint = viewPoint(sampleUv, sampleDepth);
        vec3 delta = samplePoint - point;
        float distance = length(delta);
        if (distance > 0.012 && distance < 0.85)
        {
            float facing = max(dot(normal, delta / distance), 0.0);
            visible -= facing * 0.19;
        }
    }
    return clamp(visible, 0.38, 1.0);
}

vec3 tracedReflection(vec3 point, vec3 normal, float distanceToEye)
{
    if (rayTrace < 0.5 || distanceToEye >= farPlane * 0.97) return vec3(0.0);
    vec3 viewDirection = normalize(point);
    vec3 ray = reflect(viewDirection, normal);
    if (ray.z > -0.02) return vec3(0.0);
    vec3 march = point + normal * 0.04;
    float stepSize = 0.14;
    for (int i = 0; i < 14; ++i)
    {
        march += ray * stepSize;
        stepSize *= 1.32;
        if (march.z > -0.05 || -march.z > farPlane) break;
        vec2 screenUv = projectView(march);
        if (screenUv.x < 0.0 || screenUv.x > 1.0 || screenUv.y < 0.0 || screenUv.y > 1.0) break;
        float sceneDepth = linearDepth(texture2D(depthTex, screenUv).r);
        float rayDepth = -march.z;
        if (rayDepth > sceneDepth + 0.015 && rayDepth - sceneDepth < 0.42)
        {
            float fresnel = pow(1.0 - max(dot(-viewDirection, normal), 0.0), 2.6);
            float fade = clamp(1.0 - length(march - point) / 7.0, 0.0, 1.0);
            vec3 hitColor = texture2D(scene, screenUv).rgb;
            return hitColor * fresnel * fade * (0.30 + 0.45 * max(normal.y, 0.0));
        }
    }
    return vec3(0.0);
}

vec3 volumetric(vec3 point, float distanceToEye)
{
    float glow = 0.0;
    for (int i = 0; i < 6; ++i)
    {
        float t = (float(i) + 0.5) / 6.0;
        vec3 samplePoint = point * t;
        float distance = length(samplePoint);
        vec3 direction = normalize(samplePoint);
        float cone = smoothstep(0.48, 0.95, dot(direction, vec3(0.0, 0.0, -1.0)));
        float fog = exp(-distance * 0.10);
        glow += (0.006 + cone * (0.024 + flash * 0.16)) * fog * (1.0 - t * 0.35);
    }
    glow *= 1.0 + fear * 0.4;
    return vec3(0.39, 0.47, 0.40) * glow;
}

void main()
{
    vec2 centered = uv - 0.5;
    float warp = fear * 0.0028;
    vec2 distorted = clamp(uv + vec2(sin(uv.y * 11.0 + time * 1.4),
        cos(uv.x * 9.0 + time * 1.1)) * warp, 0.0, 1.0);
    float rawDepth = texture2D(depthTex, distorted).r;
    float distanceToEye = linearDepth(rawDepth);
    vec3 point = viewPoint(distorted, distanceToEye);
    vec3 normal = surfaceNormal(point);

    // Chromatic separation near the edges: the fear response of an old lens.
    vec2 caDirection = normalize(centered + vec2(0.00001));
    float ca = (0.00055 + fear * 0.0018) * length(centered);
    vec3 color;
    color.r = texture2D(scene, clamp(distorted + caDirection * ca, 0.0, 1.0)).r;
    color.g = texture2D(scene, distorted).g;
    color.b = texture2D(scene, clamp(distorted - caDirection * ca, 0.0, 1.0)).b;

    if (distanceToEye < farPlane * 0.965)
    {
        if (rayTrace > 0.5)
        {
            color *= mix(1.0, ambientOcclusion(distorted, point, normal, distanceToEye), 0.55);
            color += tracedReflection(point, normal, distanceToEye);
        }
        color += volumetric(point, distanceToEye);
    }
    else
    {
        color += vec3(0.006, 0.010, 0.009) * (0.6 + fear);
    }

    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(color, vec3(luma), 0.05 + fear * 0.16);
    color *= mix(vec3(1.0), vec3(0.86, 0.97, 1.04), fear * 0.62);
    color += vec3(0.05, 0.07, 0.09) * (flash * 0.32 + darkness * 0.05);

    float grain = hash21((distorted + vec2(seed * 0.013, seed * 0.007)) * resolution
        + vec2(time * 61.0, time * 47.0));
    color += (grain - 0.5) * (0.036 + fear * 0.075 + threat * 0.045 + darkness * 0.015);

    float vignette = 1.0 - smoothstep(0.24, 0.86, length(centered) * 1.16);
    color *= mix(1.0, vignette, 0.42 + fear * 0.30);
    float pulse = 0.5 + 0.5 * sin(time * 2.25 + sin(time * 0.71) * 3.1);
    color *= 1.0 - fear * 0.04 * pulse;

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
