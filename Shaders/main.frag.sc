$input v_texcoord0, v_color0

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);
// x: colorize, y: alpha-test enabled, z: alpha reference, w: unused (0).
// Vec4 replaces the GL path's separate int/int/float uniforms for bgfx.
uniform vec4 u_blitParams;

void main()
{
    vec4 texel = texture2D(s_texture, v_texcoord0);
    vec4 color;
    if (u_blitParams.x != 0.0)
        color = vec4(v_color0.rgb, texel.a * v_color0.a);
    else
        color = texel * v_color0;

    if (u_blitParams.y != 0.0 && color.a < u_blitParams.z)
        discard;
    gl_FragColor = color;
}
