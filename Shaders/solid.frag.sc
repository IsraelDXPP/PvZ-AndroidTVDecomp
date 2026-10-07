$input v_texcoord0, v_color0

#include <bgfx_shader.sh>

// Declare the same varying interface as main.vert.sc, including unused UVs.
void main()
{
    gl_FragColor = v_color0;
}
