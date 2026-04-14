#ifdef GL_ES
precision mediump float;
#endif

varying vec4 v_fragmentColor;
varying vec2 v_texCoord;

uniform float u_phase;
uniform vec2 u_origin;
uniform float u_aspect;
uniform float u_focus_inner;
uniform float u_focus_outer;

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

    float focus = smoothstep(u_focus_inner, u_focus_outer, dist);
    float pattern = spikes * focus * u_phase;

    float a = pattern * v_fragmentColor.a;
    gl_FragColor = vec4(1.0, 1.0, 1.0, a);
}
