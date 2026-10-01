R""(
#version 330 core

smooth in vec2 fragTexCoords;
out vec4 color;

uniform sampler2D screenTexture;

void main() {
    color = texture(screenTexture, fragTexCoords);
}

)""