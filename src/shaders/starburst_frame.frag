#ifdef GL_ES
precision mediump float;
#endif

varying vec4 v_fragmentColor;
varying vec2 v_texCoord;

uniform float u_phase;
uniform vec2 u_origin;
uniform float u_aspect;

// Radii in aspect-normalized space (same units as length of corrected delta).
#define R_FOCUS_INNER 0.06
#define R_FOCUS_OUTER 0.14
#define RAY_COUNT 18.0
#define SPIKE_POWER 2.5

void main() {
    vec2 uv = v_texCoord;
    vec2 delta = uv - u_origin;
    delta.x *= u_aspect;
    float dist = length(delta);
    float angle = atan(delta.y, delta.x);

    float spokes = abs(sin(angle * RAY_COUNT * 0.5));
    float rayBand = pow(max(spokes, 0.001), SPIKE_POWER);
    float ambient = 0.16;
    float spikes = ambient + (1.0 - ambient) * rayBand;

    float focus = smoothstep(R_FOCUS_INNER, R_FOCUS_OUTER, dist);
    float pattern = spikes * focus * u_phase;

    vec3 tint = vec3(1.0, 0.92, 0.65);
    // Standard blending: out = src.rgb * src.a on black — use vec4(tint, a), NOT vec4(tint*a, a).
    gl_FragColor = vec4(tint, pattern) * v_fragmentColor;
}
