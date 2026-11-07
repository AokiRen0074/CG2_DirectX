struct SegmentLight {
    float4 startRadius;
    float4 endIntensity;
    float4 color;
};

struct ShadowCaster {
    float4 centerStrength;
    float4 radii;
};

struct SceneLighting {
    float4 cameraEnabled;
    float4 ambient;
    SegmentLight lights[2];
    ShadowCaster casters[16];
};

// 小規模な横スクロール用の楕円体による投影影。上向きの面だけが受ける。
// 距離とともに輪郭を柔らかくし、空中でも足場に位置関係を残す。
float GroundVisibility(SceneLighting scene, float3 position, float3 towardLight) {
    float visibility = 1;
    for (int i = 0; i < 16; ++i) {
        ShadowCaster caster = scene.casters[i];
        if (caster.centerStrength.w <= 0) {
            continue;
        }

        float3 radii = max(caster.radii.xyz, 0.05);
        float3 origin = (position - caster.centerStrength.xyz) / radii;
        float3 direction = towardLight / radii;
        float t = max(0, -dot(origin, direction) / max(dot(direction, direction), 0.0001));
        float distance = length(origin + direction * t);
        float softness = 0.15 + min(t * 0.07, 0.6);
        float occlusion = (1 - smoothstep(0.75, 1 + softness, distance)) * step(0.025, t);
        visibility *= 1 - occlusion * caster.centerStrength.w;
    }

    return visibility;
}

float3 ShadeScene(SceneLighting scene, float3 base, float3 normal, float3 position,
                  float3 keyColor, float3 keyDirection, float keyIntensity) {
    float3 N = normalize(normal);
    float3 L = -normalize(keyDirection);
    float3 V = normalize(scene.cameraEnabled.xyz - position);
    float visibility = N.y > 0.45 ? GroundVisibility(scene, position + N * 0.015, L) : 1;
    float diffuse = saturate(dot(N, L));
    float3 tint = lerp(float3(0.1, 0.1, 0.1), keyColor, 0.9);
    float3 light = scene.ambient.rgb * tint + keyColor * diffuse * keyIntensity * 0.72 * visibility;
    // 弱い青い補助光で、正面の輪郭を暗闇に沈めない。
    light += float3(0.08, 0.13, 0.22) * tint * saturate(dot(N, normalize(float3(0.6, 0.1, -1))));
    float3 result = base * light * lerp(0.78, 1, visibility);
    for (int i = 0; i < 2; ++i) {
        SegmentLight source = scene.lights[i];
        if (source.endIntensity.w <= 0) {
            continue;
        }

        float3 segment = source.endIntensity.xyz - source.startRadius.xyz;
        float t = saturate(dot(position - source.startRadius.xyz, segment) / max(dot(segment, segment), 0.0001));
        float3 delta = source.startRadius.xyz + segment * t - position;
        float distance = length(delta);
        float3 direction = delta / max(distance, 0.001);
        float attenuation = pow(saturate(1 - distance / max(source.startRadius.w, 0.01)), 2);
        float energy = source.endIntensity.w * attenuation;
        float facing = saturate(dot(N, direction));
        float3 halfDirection = normalize(direction + V + float3(0, 0, 0.0001));
        float specular = pow(saturate(dot(N, halfDirection)), 24) * 0.12;
        result += ((base + 0.025) * facing + specular) * source.color.rgb * energy;
    }

    return result;
}
