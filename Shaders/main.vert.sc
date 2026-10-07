$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_color0

#include <bgfx_shader.sh>

// GLShader::kVertexSrc: affine transform, then pixel space to clip space.
// Upload u_transform in column-major order; u_viewport.xy is width/height.
uniform mat3 u_transform;
uniform vec4 u_viewport;

void main()
{
    vec3 position = mul(u_transform, vec3(a_position, 1.0));
    vec2 clipPosition = vec2(
        position.x / u_viewport.x * 2.0 - 1.0,
        1.0 - position.y / u_viewport.y * 2.0
    );
    gl_Position = vec4(clipPosition, 0.0, 1.0);
    v_texcoord0 = a_texcoord0;
    // GLGraphics::Flush @0x4762c8: the per-vertex colour modulates the texel.
    v_color0 = a_color0;
}
