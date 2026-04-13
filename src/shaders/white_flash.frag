#ifdef GL_ES
precision lowp float;
#endif
varying vec2 v_texCoord;
uniform sampler2D CC_Texture0;
void main() {
    float a = texture2D(CC_Texture0, v_texCoord).a;
    gl_FragColor = vec4(a, a, a, a);
}
