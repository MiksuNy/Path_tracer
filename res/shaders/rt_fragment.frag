#version 460

#define PI 3.14159265
#define TWO_PI 6.28318530
#define INFINITY 10000000.0

// Random number functions: https://github.com/SebLague/Ray-Tracing/blob/main/Assets/Scripts/Shaders/RayTracing.shader

in vec2 accumTexCoords;
out vec4 fragColor;

uniform sampler2D accumTexture;

vec2 viewport = vec2(gl_FragCoord.xy / textureSize(accumTexture, 0));
vec2 viewportCenter = viewport - 0.5;

uniform int currAccumPass;

uniform bool debugNormal;

struct Ray { vec3 origin; vec3 direction; };
struct Camera {
	vec3 position;
	mat4 inverseView;
};
struct Material {
	vec4 baseColor;
	vec4 specularColor;
	vec4 emissionColor;
    float smoothness;
    float specularSmoothness;
	float emissionStrength;
	float ior;
	float refractionAmount;
	float specularChance;
};
struct Sphere { vec3 position; float radius; uint materialIndex; };
struct Triangle { vec4 p1; vec4 p2; vec4 p3; uint materialIndex; }; // 64 bytes = 52 bytes + 12 bytes of padding
struct HitInfo { vec3 hitPoint; vec3 hitNormal; float hitDist; float travelDist; bool hasHit; bool frontFace; Material hitMaterial; };
struct Node { vec4 boundsMin; vec4 boundsMax; int triIndex; int numTris; int childrenIndex; };

uniform Camera cam;

layout (std430, binding = 1) readonly buffer meshSSBO {
	Triangle meshTriangles[];
};
layout (std430, binding = 2) readonly buffer bvhSSBO {
	Node nodes[];
};
layout (std430, binding = 3) readonly buffer materialSSBO {
	Material sceneMaterials[];
};
layout (std430, binding = 4) readonly buffer sphereSSBO {
	Sphere sceneSpheres[];
};
layout (std430, binding = 5) readonly buffer triangleSSBO {
	Triangle sceneTriangles[];
};

uint NextRandom(inout uint state) {
	state = state * 747796405 + 2891336453;
	uint result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
	result = (result >> 22) ^ result;
	return result;
}

float RandomValue(inout uint state) {
	return NextRandom(state) / 4294967295.0; // 2^32 - 1
}

float RandomValueND(inout uint state) {
	float theta = TWO_PI * RandomValue(state);
	float rho = sqrt(-2 * log(RandomValue(state)));
	return rho * cos(theta);
}

vec3 RandomDirection(inout uint state) {
	float x = RandomValueND(state);
	float y = RandomValueND(state);
	float z = RandomValueND(state);
	return normalize(vec3(x, y, z));
}

vec3 RandomInHemisphere(in vec3 normal, inout uint state) {
	vec3 randomInSphere = RandomDirection(state);
	return dot(randomInSphere, normal) > 0.0 ? randomInSphere : -randomInSphere;
}

float LengthSquared(in vec3 vec) {
	return (vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

// https://tavianator.com/2011/ray_box.html
bool HitAABB(in vec3 boundsMin, in vec3 boundsMax, in Ray ray) {
	vec3 invDir = (1.0/ray.direction);

	float tx1 = (boundsMin.x - ray.origin.x)*invDir.x;
	float tx2 = (boundsMax.x - ray.origin.x)*invDir.x;

	float tMin = min(tx1, tx2);
	float tMax = max(tx1, tx2);

	float ty1 = (boundsMin.y - ray.origin.y)*invDir.y;
	float ty2 = (boundsMax.y - ray.origin.y)*invDir.y;

	tMin = max(tMin, min(ty1, ty2));
	tMax = min(tMax, max(ty1, ty2));

	float tz1 = (boundsMin.z - ray.origin.z)*invDir.z;
	float tz2 = (boundsMax.z - ray.origin.z)*invDir.z;

	tMin = max(tMin, min(tz1, tz2));
	tMax = min(tMax, max(tz1, tz2));

	return tMax >= tMin && tMax >= 0;
};

HitInfo HitSphere(in Sphere sphere, in Ray ray) {
	HitInfo tempHitInfo;

    vec3 oc = vec3(ray.origin - sphere.position);
    float a = LengthSquared(ray.direction);
    float halfB = dot(oc, ray.direction);
    float c = LengthSquared(oc) - sphere.radius * sphere.radius;
    
	float discriminant = halfB * halfB - a * c;

	float t0 = (-halfB - sqrt(discriminant)) / a;
	float t1 = (-halfB + sqrt(discriminant)) / a;

	tempHitInfo.hasHit = t0 > 0.0 || t1 > -0.0;
	tempHitInfo.frontFace = t0 > 0.0;
	tempHitInfo.hitPoint = t0 > 0.0 ? ray.origin + ray.direction * t0 : ray.origin + ray.direction * t1;
	tempHitInfo.hitDist = t0 > 0.0 ? t0 : t1;
	tempHitInfo.travelDist = t0 > 0.0 ? t1 - t0 : t1 - t0;
	tempHitInfo.hitNormal = t0 > 0.0 ? normalize(tempHitInfo.hitPoint - sphere.position) : -normalize(tempHitInfo.hitPoint - sphere.position);
	tempHitInfo.hitMaterial = sceneMaterials[sphere.materialIndex];
	return tempHitInfo;
}

// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
HitInfo HitTriangle(in Triangle tri, in Ray ray) {
	HitInfo tempHitInfo;
	tempHitInfo.hasHit = false;

	vec3 edge1 = tri.p2.xyz - tri.p1.xyz;
	vec3 edge2 = tri.p3.xyz - tri.p1.xyz;
	vec3 rayCrossE2 = cross(ray.direction, edge2);
	float det = dot(edge1, rayCrossE2);

	if (det < 0.0 && det > -0.0) return tempHitInfo;

	float invDet = 1.0 / det;
	vec3 s = ray.origin - tri.p1.xyz;
	float u = invDet * dot(s, rayCrossE2);

	if (u < 0 || u > 1) return tempHitInfo;

	vec3 sCrossE1 = cross(s, edge1);
	float v = invDet * dot(ray.direction, sCrossE1);

	if (v < 0 || u + v > 1) return tempHitInfo;

	float t = invDet * dot(edge2, sCrossE1);

	tempHitInfo.hasHit = t > 0.0;
	tempHitInfo.frontFace = !(det < 0.0);
	tempHitInfo.hitPoint = ray.origin + ray.direction * t;
	tempHitInfo.hitDist = t;
	tempHitInfo.hitNormal = det < 0.0 ? normalize(cross(edge2, edge1)) : normalize(cross(edge1, edge2)); // Determine tri normal by which side of the tri was hit
	tempHitInfo.hitMaterial = sceneMaterials[tri.materialIndex];
	return tempHitInfo;
}

HitInfo TraverseBVH(in Ray ray) {
	HitInfo result;
	result.hitDist = INFINITY;

	Node nodeStack[6];
	int stackIndex = 0;
	nodeStack[stackIndex++] = nodes[0];

	while (stackIndex > 0) {
		Node node = nodeStack[--stackIndex];

		if (HitAABB(node.boundsMin.xyz, node.boundsMax.xyz, ray)) {
			if (node.childrenIndex == 0) {
				for (int i = node.triIndex; i < node.triIndex + node.numTris; ++i) {
					HitInfo triHit = HitTriangle(meshTriangles[i], ray);
					if (triHit.hasHit && triHit.hitDist < result.hitDist) result = triHit;
				}
			} else {
				nodeStack[stackIndex++] = nodes[node.childrenIndex + 1];
				nodeStack[stackIndex++] = nodes[node.childrenIndex];
			}
		}
	}

	return result;
}

HitInfo CalculateRay(in Ray ray) {
	HitInfo closestHit;
	closestHit.hitDist = INFINITY;
	HitInfo tempHit;

	// New mesh triangle test with BVH
	//closestHit = TraverseBVH(ray);

	// Old mesh triangle test
	if (HitAABB(nodes[0].boundsMin.xyz, nodes[0].boundsMax.xyz, ray)) {
		for (int i = 0; i < meshTriangles.length(); ++i) {
			tempHit = HitTriangle(meshTriangles[i], ray);
			if (tempHit.hasHit && tempHit.hitDist < closestHit.hitDist) closestHit = tempHit;
		}
	}

	for (int i = 0; i < sceneTriangles.length(); ++i) {
		tempHit = HitTriangle(sceneTriangles[i], ray);
		if (tempHit.hasHit && tempHit.hitDist < closestHit.hitDist) closestHit = tempHit;
	}

	for (int i = 0; i < sceneSpheres.length(); ++i) {
		tempHit = HitSphere(sceneSpheres[i], ray);
		if (tempHit.hasHit && tempHit.hitDist < closestHit.hitDist) closestHit = tempHit;
	}

	return closestHit;
}

// https://blog.demofox.org/2017/01/09/raytracing-reflection-refraction-fresnel-total-internal-reflection-and-beers-law/
float FresnelReflectAmount(in float n1, in float n2, in vec3 normal, in vec3 incident, in float reflectivity)
{
        // Schlick aproximation
        float r0 = (n1-n2) / (n1+n2);
        r0 *= r0;
        float cosX = -dot(normal, incident);
        if (n1 > n2)
        {
            float n = n1/n2;
            float sinT2 = n*n*(1.0-cosX*cosX);
            // Total internal reflection
            if (sinT2 > 1.0)
                return 1.0;
            cosX = sqrt(1.0-sinT2);
        }
        float x = 1.0-cosX;
        float ret = r0+(1.0-r0)*x*x*x*x*x;
		ret = (reflectivity + (1.0-reflectivity) * ret);
		return ret;
}

const vec3 skyColor = vec3(0.92, 0.8, 0.85);
const float skyIntensity = 0.8;

vec3 RayTrace(in Ray ray, in int maxBounces, inout uint state) {
	vec3 rayColor = vec3(1);
	vec3 incomingLight = vec3(0);
	vec3 emittedLight = vec3(0);

	// BVH visualisation
//	for (int i = 0; i < nodes.length(); ++i) {
//		float color = 0.01;
//		if (nodes[i].childrenIndex == 0) {
//			if (HitAABB(nodes[i].boundsMin.xyz, nodes[i].boundsMax.xyz, ray)) incomingLight.xyz += ray.direction;
//		}
//	}

	// Raytracing
	int currBounces = 0;
	for (int i = 0; i < maxBounces; ++i) {
		HitInfo hitInfo = CalculateRay(ray);
		if (hitInfo.hasHit && hitInfo.hitDist < INFINITY) {
			if (debugNormal) return hitInfo.hitNormal;

			currBounces++;
			
			ray.origin = hitInfo.hitPoint;
			
			float ior = hitInfo.frontFace ? (1.0 / hitInfo.hitMaterial.ior) : hitInfo.hitMaterial.ior;

			bool isSpecularBounce = hitInfo.hitMaterial.specularChance > RandomValue(state);
			float fresnel = FresnelReflectAmount(1.0, hitInfo.hitMaterial.ior, hitInfo.hitNormal, ray.direction, 1.0 - hitInfo.hitMaterial.refractionAmount);

			bool isRefracted = fresnel < RandomValue(state);

			ray.direction = normalize (
				mix (
					mix (
						RandomInHemisphere(hitInfo.hitNormal, state),
						reflect(ray.direction, hitInfo.hitNormal),
						isSpecularBounce ? hitInfo.hitMaterial.specularSmoothness : hitInfo.hitMaterial.smoothness
					),
					mix (
						-RandomInHemisphere(hitInfo.hitNormal, state),
						refract(ray.direction, hitInfo.hitNormal, ior),
						hitInfo.hitMaterial.smoothness
					),
					isRefracted
				)
			);

			ray.origin += ray.direction * (0.0002 * RandomValue(state)); // Make sure ray doesn't collide again on the same point

			emittedLight += hitInfo.hitMaterial.emissionColor.rgb * hitInfo.hitMaterial.emissionStrength;
			rayColor *= mix(mix(hitInfo.hitMaterial.baseColor.rgb, hitInfo.hitMaterial.specularColor.rgb, isSpecularBounce), hitInfo.hitMaterial.baseColor.rgb, isRefracted);
			//vec3 absorb = exp(-hitInfo.hitMaterial.baseColor.rgb * hitInfo.travelDist); // Beer's law
			//rayColor *= mix(vec4(1), vec4(absorb, 1.0), isRefracted);
			incomingLight += emittedLight * rayColor;
		} else {
			currBounces++;
			
			emittedLight += skyColor * skyIntensity;
			rayColor *= skyColor;
			incomingLight += emittedLight * rayColor;
			break;
		}
	}

	return sqrt(incomingLight.rgb / currBounces);
}

void main() {
	vec2 accumTexSize = textureSize(accumTexture, 0);
	
	uint pixelIndex = uint(viewport.x / accumTexSize.x * 4294967295.0 + viewport.y / accumTexSize.y * 4294967295.0);
	uint rngState = uint(currAccumPass * pixelIndex);

	Ray ray;
	ray.origin = cam.position;

	vec3 jitter = RandomDirection(rngState) * 0.0005; // For anti-aliasing
	vec2 pixelPos = vec2(viewportCenter.x * accumTexSize.x / accumTexSize.y, viewportCenter.y);

	ray.direction = normalize(vec3(cam.inverseView * vec4(-pixelPos.x + jitter.x, pixelPos.y + jitter.y, 1.0, 0.0)));

	int maxBounces = debugNormal ? 1 : 8;
	vec3 rayTraceColor = RayTrace(ray, maxBounces, rngState);

	vec3 finalColor = mix(texture(accumTexture, accumTexCoords).rgb, rayTraceColor, 1.0 / float(currAccumPass));

	fragColor = vec4(finalColor, 1.0);
}