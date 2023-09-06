
float computeLuminance(vec3 color) {
    // RGB to YUV weights for luminance calculation
    const vec3 W = vec3(0.2126, 0.7152, 0.0722);
    return dot(color, W);
}

vec3 colorRamp2(float value, vec3 color0, vec3 color1) {
    return mix(color0, color1, value);
}

vec3 colorRamp5(float value, vec3 color0, vec3 color1, vec3 color2, vec3 color3, vec3 color4) {
    if (value == 0.0)
        return vec3(0.0);
    float knot0 = 0.0;
    float knot1 = 0.2;  // violet
    float knot2 = 0.4;  // blue
    float knot3 = 0.8;  // red
    float knot4 = 1.0;  // yellow
    if (value < knot0) {
        return color0;
    } else if (value < knot1) {
        float t = (value - knot0) / (knot1 - knot0);
        return mix(color0, color1, t);
    } else if (value < knot2) {
        float t = (value - knot1) / (knot2 - knot1);
        return mix(color1, color2, t);
    } else if (value < knot3) {
        float t = (value - knot2) / (knot3 - knot2);
        return mix(color2, color3, t);
    } else if (value < knot4) {
        float t = (value - knot3) / (knot4 - knot3);
        return mix(color3, color4, t);
    } else {
        return color4;
    }
}

vec3 toneMapping(vec3 color, float exposure) {
    color = vec3(1.0) - exp(-color * exposure);
    return color;
}

vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 saturate(vec3 color, float saturation) {
    vec3 hsv = rgb2hsv(color);
    hsv.y *= saturation;  // saturate
    return hsv2rgb(hsv);
}

// source: https://github.com/selfshadow/ltc_code/blob/master/webgl/shaders/ltc/ltc_blit.fs
vec3 RRTAndODTFit(vec3 v) {
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 toneMappingACESFilmic(vec3 color, float exposure) {
    // sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
    const mat3 ACESInputMat =
        mat3(vec3(0.59719, 0.07600, 0.02840),  // transposed from source
             vec3(0.35458, 0.90834, 0.13383), vec3(0.04823, 0.01566, 0.83777));

    // ODT_SAT => XYZ => D60_2_D65 => sRGB
    const mat3 ACESOutputMat =
        mat3(vec3(1.60475, -0.10208, -0.00327),  // transposed from source
             vec3(-0.53108, 1.10813, -0.07276), vec3(-0.07367, -0.00605, 1.07602));

    color *= exposure / 0.6;

    color = ACESInputMat * color;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = ACESOutputMat * color;

    color = saturate(color, 1.3);

    // Clamp to [0, 1]
    return clamp(color, 0.0, 1.0);
}
