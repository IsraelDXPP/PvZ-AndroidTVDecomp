$input v_texcoord0, v_color0

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);
// x: colorize, y: alpha-test enabled, z: alpha reference, w: FilterEffect + 1.
// Vec4 replaces the GL path's separate int/int/float uniforms for bgfx.
uniform vec4 u_blitParams;

vec3 filterLumSat(vec3 rgb, float lightnessScale, float saturationScale)
{
    // FilterEffectDoLumSat @0x206a38, with the same HSL scales as software.
    float high = max(rgb.r, max(rgb.g, rgb.b));
    float low = min(rgb.r, min(rgb.g, rgb.b));
    float lightness = (high + low) * 0.5;
    float range = high - low;
    float saturation = 0.0;
    float hue = 0.0;
    if (range > 0.0)
    {
        saturation = range / (lightness > 0.5 ? 2.0 - high - low : high + low);
        if (high == rgb.r)
            hue = (rgb.g - rgb.b) / range + (rgb.g < rgb.b ? 6.0 : 0.0);
        else if (high == rgb.g)
            hue = (rgb.b - rgb.r) / range + 2.0;
        else
            hue = (rgb.r - rgb.g) / range + 4.0;
        hue *= 0.16667;
    }
    lightness *= lightnessScale;
    saturation *= saturationScale;
    high = lightness <= 0.5 ? lightness * (1.0 + saturation)
        : lightness + saturation - lightness * saturation;
    low = 2.0 * lightness - high;
    float hueSector = hue * 6.0;
    float sector = floor(hueSector);
    float fraction = hueSector - sector;
    float rising = low + (high - low) * fraction;
    float falling = high - (high - low) * fraction;
    vec3 result;
    if (sector < 1.0) result = vec3(high, rising, low);
    else if (sector < 2.0) result = vec3(falling, high, low);
    else if (sector < 3.0) result = vec3(low, high, rising);
    else if (sector < 4.0) result = vec3(low, falling, high);
    else if (sector < 5.0) result = vec3(rising, low, high);
    else result = vec3(high, low, falling);
    return floor(clamp(result, vec3_splat(0.0), vec3_splat(1.0)) * 255.0) / 255.0;
}

void main()
{
    vec4 texel = texture2D(s_texture, v_texcoord0);
    if (u_blitParams.w > 0.0 && texel.a > 0.0)
    {
        vec3 rgb = texel.rgb / texel.a;
        if (u_blitParams.w < 1.5)
            rgb = filterLumSat(rgb, 1.8, 0.2);
        else if (u_blitParams.w < 2.5)
            rgb = filterLumSat(rgb, 1.2, 0.3);
        else
            rgb = vec3_splat(1.0);
        texel.rgb = rgb * texel.a;
    }
    vec4 color;
    if (u_blitParams.x != 0.0)
        color = vec4(v_color0.rgb, texel.a * v_color0.a);
    else
        color = texel * v_color0;

    if (u_blitParams.y != 0.0 && color.a < u_blitParams.z)
        discard;
    gl_FragColor = color;
}
