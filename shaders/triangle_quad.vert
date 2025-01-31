#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

// Vulkan-style push constants
layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

void main() {
    // Vulkan clip space coordinates (Y points down, depth [0,1])
    gl_Position = pc.mvp * vec4(inPosition, 0.0, 1.0);
    gl_Position.y = -gl_Position.y; // Flip Y-axis if needed
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
    
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}