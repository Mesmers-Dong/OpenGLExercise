#version 330 core

in vec2 Texcoords;

uniform sampler2D screenTexture;

out vec4 FragColor;

void main() {
    FragColor = texture(screenTexture,Texcoords);
}
