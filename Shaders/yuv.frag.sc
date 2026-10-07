$input v_texcoord0, v_color0

#include <bgfx_shader.sh>

// GLRenderDevice::DrawYuv: three R8 planes, sampler stages 0/1/2.
SAMPLER2D(s_y, 0);
SAMPLER2D(s_cb, 1);
SAMPLER2D(s_cr, 2);

void main()
{
    // Preserve GLShader::kYuvFragSrc's BT.601 limited-range conversion.
    float luminance = texture2D(s_y, v_texcoord0).r;
    float blueChroma = texture2D(s_cb, v_texcoord0).r - 0.5;
    float redChroma = texture2D(s_cr, v_texcoord0).r - 0.5;
    luminance = (luminance - 0.0627451) * 1.1643836;
    gl_FragColor = vec4(
        luminance + 1.5960268 * redChroma,
        luminance - 0.3917623 * blueChroma - 0.8129676 * redChroma,
        luminance + 2.0172321 * blueChroma,
        1.0
    );
}
