#version 460

#define PI 3.14159265
#define TWO_PI 6.28318530
#define INFINITY 10000000.0

// Random number functions: https://github.com/SebLague/Ray-Tracing/blob/main/Assets/Scripts/Shaders/RayTracing.shader

in vec2 accumTexCoords;
out vec4 fragColor;

uniform int SCREEN_W;
uniform int SCREEN_H;

vec2 viewport = vec2(gl_FragCoord.xy / vec2(SCREEN_W, SCREEN_H));
vec2 viewportCenter = viewport - 0.5;

uniform float frameTime;
uniform float currAccumPass;

uniform sampler2D accumTexture;

struct Ray { vec3 origin; vec3 direction; };
struct Camera { vec3 position; };
struct Material { vec3 baseColor; float roughness; vec3 emissionColor; float emissionStrength; float ior; float refractionAmount; };
struct Sphere { vec3 position; float radius; Material material; };
struct Triangle { vec3 p1; vec3 p2; vec3 p3; Material material; };
struct HitInfo { vec3 hitPoint; vec3 hitNormal; float hitDist; bool hasHit; Material hitMaterial; };
struct Node { vec4 boundsMin; vec4 boundsMax; int firstVertexIndex; int numVertices; int padding[2]; };

layout (std430, binding = 1) buffer vertexSSBO {
	vec4 vertices[];
};
layout (std430, binding = 2) buffer indexSSBO {
	ivec4 indices[];
};
layout (std430, binding = 3) buffer BVH {
	vec4 mesh_boundsMin;
	vec4 mesh_boundsMax;
	Node childA;
	Node childB;
};

uniform Camera cam;

uniform Sphere sphere1;
uniform Sphere sphere2;
uniform Sphere sphere3;
uniform Sphere sunSphere;

Sphere spheres[] = {
	sphere1, sphere2, sphere3, sunSphere
};

uniform Triangle tri1;
uniform Triangle tri2;
uniform Triangle tri3;

Triangle tris[] = {
	tri1, tri2, tri3
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

vec3 RandomInHemisphere(vec3 normal, inout uint state) {
	vec3 randomInSphere = RandomDirection(state);
	if (dot(randomInSphere, normal) >= 0.0) {
		return randomInSphere;
	} else {
		return -randomInSphere;
	}
}

float LengthSquared(vec3 vec) {
	return (vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

// Thanks to https://tavianator.com/2011/ray_box.html
bool HitAABB(vec3 boundsMin, vec3 boundsMax, in Ray ray) {
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

	return tMax >= tMin && tMax > 0;
};

HitInfo HitSphere(vec3 center, float radius, Ray ray) {
	HitInfo tempHitInfo;

    vec3 oc = vec3(ray.origin - center);
    float a = LengthSquared(ray.direction);
    float halfB = dot(oc, ray.direction);
    float c = LengthSquared(oc) - radius * radius;
    
	float discriminant = halfB * halfB - a * c;

	float t = (-halfB - sqrt(discriminant)) / a;

	tempHitInfo.hasHit = t > 0.00001;
	tempHitInfo.hitPoint = ray.origin + ray.direction * t;
	tempHitInfo.hitDist = t;
	tempHitInfo.hitNormal = normalize(tempHitInfo.hitPoint - center);
	return tempHitInfo;
}

// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
HitInfo HitTriangle(Triangle tri, in Ray ray) {
	HitInfo tempHitInfo;

	vec3 edge1 = tri.p2 - tri.p1;
	vec3 edge2 = tri.p3 - tri.p1;
	vec3 rayCrossE2 = cross(ray.direction, edge2);
	float det = dot(edge1, rayCrossE2);

	float invDet = 1.0 / det;
	vec3 s = ray.origin - tri.p1;
	float u = invDet * dot(s, rayCrossE2);

	vec3 sCrossE1 = cross(s, edge1);
	float v = invDet * dot(ray.direction, sCrossE1);

	float t = invDet * dot(edge2, sCrossE1);

	tempHitInfo.hasHit = t > 0 && !(det < 0) && !(u < 0 || u > 1) && !(v < 0 || u + v > 1);
	tempHitInfo.hitPoint = ray.origin + ray.direction * t;
	tempHitInfo.hitDist = t;
	tempHitInfo.hitNormal = normalize(cross(edge1, edge2));
	return tempHitInfo;
}

HitInfo CalculateRay(in Ray ray) {
	HitInfo closestHit;
	closestHit.hitDist = INFINITY;
	HitInfo tempHit;

	Material tempMaterial;
	tempMaterial.baseColor = vec3(0.5, 1.0, 0.5);
	tempMaterial.roughness = 0.5f;

	Triangle tempTri;
	if (HitAABB(mesh_boundsMin.xyz, mesh_boundsMax.xyz, ray)) {
		for (int i = 0; i < indices.length(); ++i) {
			tempTri.p1 = vertices[indices[i].x].xyz;
			tempTri.p2 = vertices[indices[i].y].xyz;
			tempTri.p3 = vertices[indices[i].z].xyz;

			tempHit = HitTriangle(tempTri, ray);
			tempHit.hitMaterial = tempMaterial;

			if (tempHit.hasHit && tempHit.hitDist < closestHit.hitDist) {
				closestHit = tempHit;
				closestHit.hitMaterial = tempHit.hitMaterial;
			}
		}
	}


	for (int i = 0; i < tris.length(); ++i) {
		tempHit = HitTriangle(tris[i], ray);
		tempHit.hitMaterial = tris[i].material;

		if (tempHit.hasHit && tempHit.hitDist < closestHit.hitDist) {
			closestHit = tempHit;
			closestHit.hitMaterial = tempHit.hitMaterial;
		}
	}

	for (int i = 0; i < spheres.length(); ++i) {
		tempHit = HitSphere(spheres[i].position, spheres[i].radius, ray);
		tempHit.hitMaterial = spheres[i].material;

		if (tempHit.hasHit && tempHit.hitDist < closestHit.hitDist) {
			closestHit = tempHit;
			closestHit.hitMaterial = tempHit.hitMaterial;
		}
	}
	return closestHit;
}

vec3 RayTrace(in Ray ray, int maxBounces, inout uint state) {
	vec3 rayColor = vec3(1);
	vec3 incomingLight = vec3(0);
	vec3 emittedLight = vec3(0);

	vec3 skyColor = vec3(1);
	float skyIntensity = 0.0;

	int currBounces = 0;

	// BVH visualisation
	if (HitAABB(mesh_boundsMin.xyz, mesh_boundsMax.xyz, ray)) incomingLight += vec3(1.0, 0.0, 0.0);
	if (HitAABB(childA.boundsMin.xyz, childA.boundsMax.xyz, ray)) incomingLight += vec3(0.0, 1.0, 0.0);
	if (HitAABB(childB.boundsMin.xyz, childB.boundsMax.xyz, ray)) incomingLight += vec3(0.0, 0.0, 1.0);

	// Raytracing
	for (int i = 0; i < maxBounces; ++i) {
		HitInfo hitInfo = CalculateRay(ray);
		if (hitInfo.hasHit && hitInfo.hitDist < INFINITY) {
			currBounces++;

			ray.origin = hitInfo.hitPoint;

			ray.direction = normalize (
				mix (
					mix(reflect(ray.direction, hitInfo.hitNormal), RandomInHemisphere(hitInfo.hitNormal, state), hitInfo.hitMaterial.roughness),
					mix(refract(ray.direction, hitInfo.hitNormal, hitInfo.hitMaterial.ior), -RandomInHemisphere(hitInfo.hitNormal, state), hitInfo.hitMaterial.roughness),
					hitInfo.hitMaterial.refractionAmount
				)
			);

			emittedLight += hitInfo.hitMaterial.emissionColor * hitInfo.hitMaterial.emissionStrength;
			rayColor *= hitInfo.hitMaterial.baseColor; 
			incomingLight += (emittedLight * rayColor) * dot(hitInfo.hitNormal, ray.direction);
		} else {
			currBounces++;
			
			emittedLight += skyColor * skyIntensity;
			rayColor *= emittedLight;
			incomingLight += rayColor;
			break;
		}
	}

	return incomingLight / currBounces;
}

void main() {
	uint pixelIndex = uint(viewport.x / SCREEN_W * 4294967295.0 + viewport.y / SCREEN_H * 4294967295.0);
	uint rngState = uint(pixelIndex * 1234 * frameTime);

	Ray ray;
	ray.origin = cam.position;

	vec3 camForward = vec3(cam.position.xy, cam.position.z - 1.0);

	ray.direction = vec3(vec2(viewportCenter.x * SCREEN_W/SCREEN_H, viewportCenter.y) + camForward.xy, camForward.z) - ray.origin;

	vec3 finalColor = vec3(0);
	vec3 rayTraceColor = vec3(0);

	int maxBounces = 2;
	rayTraceColor = RayTrace(ray, maxBounces, rngState);


	float weight = 1.0 / (currAccumPass + 1);
	finalColor = texture(accumTexture, accumTexCoords).xyz * (1 - weight) + rayTraceColor * weight;

	fragColor = vec4(finalColor, 1.0);
}