#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

// Vulkan descriptor set binding
layout(set = 0, binding = 0) uniform sampler2D texSampler;

void main() {
    // Vulkan texture sampling
    vec4 texColor = texture(texSampler, fragTexCoord);
    outColor = texColor * vec4(fragColor, 1.0);
    
    // Vulkan-specific alpha discard
    if (outColor.a < 0.1) {
        discard;
    }
}