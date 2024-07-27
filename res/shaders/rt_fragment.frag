#version 460

#define PI 3.14159265
#define TWO_PI 6.28318530
#define INFINITY 10000000.0
#define EPS 0.000001

const vec4 skyColor = vec4(0.96, 0.95, 1.0, 1.0);
const float skyIntensity = 1.0;

// Random number functions: https://github.com/SebLague/Ray-Tracing/blob/main/Assets/Scripts/Shaders/RayTracing.shader

in vec2 accumTexCoords;
out vec4 fragColor;

uniform sampler2D accumTexture;

vec2 viewport = vec2(gl_FragCoord.xy / textureSize(accumTexture, 0));
vec2 viewportCenter = viewport - 0.5;

uniform float frameTime;
uniform float currAccumPass;



struct Ray { vec3 origin; vec3 direction; };
struct Camera { vec3 position; };
struct Material {
	vec4 baseColor;
	vec4 specularColor;
	vec4 emissionColor;
	float roughness;
	float emissionStrength;
	float ior;
	float refractionAmount;
	float specularChance;

	float pad[3];
};
struct Sphere { vec3 position; float radius; Material material; };
struct Triangle { vec4 p1; vec4 p2; vec4 p3; Material material; };
struct HitInfo { vec3 hitPoint; vec3 hitNormal; float hitDist; float travelDist; bool hasHit; Material hitMaterial; };
struct Node { vec4 boundsMin; vec4 boundsMax; };

uniform Camera cam;

layout (std430, binding = 1) buffer meshSSBO {
	Triangle meshTriangles[];
};
layout (std430, binding = 2) buffer BVH {
	Node nodes[];
};

uniform Sphere sphere1;
uniform Sphere sphere2;
Sphere spheres[] = {
	sphere1, sphere2
};

uniform Triangle tri1;
Triangle tris[] = {
	tri1
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
	if (dot(randomInSphere, normal) > 0.0) {
		return randomInSphere;
	} else {
		return -randomInSphere;
	}
}

float LengthSquared(vec3 vec) {
	return (vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

// https://blog.demofox.org/2017/01/09/raytracing-reflection-refraction-fresnel-total-internal-reflection-and-beers-law/
float FresnelReflectAmount (float n1, float n2, vec3 normal, vec3 incident, float roughness)
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
 
        // adjust reflect multiplier for object reflectivity
        ret = (roughness + (1.0-roughness) * ret);
        return ret;
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

	tempHitInfo.hasHit = t > 0.0;
	tempHitInfo.hitPoint = ray.origin + ray.direction * t;
	tempHitInfo.hitDist = t;
	tempHitInfo.hitNormal = normalize(tempHitInfo.hitPoint - center);
	return tempHitInfo;
}

// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
HitInfo HitTriangle(Triangle tri, in Ray ray) {
	HitInfo tempHitInfo;

	vec3 edge1 = tri.p2.xyz - tri.p1.xyz;
	vec3 edge2 = tri.p3.xyz - tri.p1.xyz;
	vec3 rayCrossE2 = cross(ray.direction, edge2);
	float det = dot(edge1, rayCrossE2);

	float invDet = 1.0 / det;
	vec3 s = ray.origin - tri.p1.xyz;
	float u = invDet * dot(s, rayCrossE2);

	vec3 sCrossE1 = cross(s, edge1);
	float v = invDet * dot(ray.direction, sCrossE1);

	float t = invDet * dot(edge2, sCrossE1);

	tempHitInfo.hasHit = (t > 0.0) && !(det < 0) && !(u < 0 || u > 1) && !(v < 0 || u + v > 1);
	tempHitInfo.hitPoint = ray.origin + ray.direction * t;
	tempHitInfo.hitDist = t;
	tempHitInfo.hitNormal = normalize(cross(edge1, edge2));
	tempHitInfo.hitMaterial = tri.material;
	return tempHitInfo;
}

HitInfo CalculateRay(in Ray ray) {
	HitInfo closestHit;
	closestHit.hitDist = INFINITY;
	HitInfo tempHit;

	if (HitAABB(nodes[0].boundsMin.xyz, nodes[0].boundsMax.xyz, ray)) {
		for (int i = 0; i < meshTriangles.length(); ++i) {
			tempHit = HitTriangle(meshTriangles[i], ray);

			if (tempHit.hasHit && tempHit.hitDist < closestHit.hitDist) {
				closestHit.hasHit = true;
				closestHit = tempHit;
			}
		}
	}

	for (int i = 0; i < tris.length(); ++i) {
		tempHit = HitTriangle(tris[i], ray);

		if (tempHit.hasHit && tempHit.hitDist < closestHit.hitDist) {
			closestHit.hasHit = true;
			closestHit = tempHit;
		}
	}

	for (int i = 0; i < spheres.length(); ++i) {
		tempHit = HitSphere(spheres[i].position, spheres[i].radius, ray);
		tempHit.hitMaterial = spheres[i].material;

		if (tempHit.hasHit && tempHit.hitDist < closestHit.hitDist) {
			closestHit.hasHit = true;
			closestHit = tempHit;
			closestHit.hitMaterial = tempHit.hitMaterial;
		}
	}
	return closestHit;
}

vec3 RayTrace(in Ray ray, int maxBounces, inout uint state) {
	vec4 rayColor = vec4(1);
	vec4 incomingLight = vec4(0);
	vec4 emittedLight = vec4(0);

	// BVH visualisation
//	for (uint i = 0; i < nodes.length(); ++i) {
//		if (HitAABB(nodes[i].boundsMin.xyz, nodes[i].boundsMax.xyz, ray)) incomingLight += vec3(0.2, 0.0, 0.0);
//	}

	// Raytracing
	int currBounces = 0;
	for (int i = 0; i < maxBounces; ++i) {
		HitInfo hitInfo = CalculateRay(ray);
		if (hitInfo.hasHit && hitInfo.hitDist < INFINITY) {
			currBounces++;

			ray.origin = hitInfo.hitPoint;
			
			bool isSpecularBounce = hitInfo.hitMaterial.specularChance > RandomValue(state);

			ray.direction = normalize (
				mix (
					mix (
						reflect(ray.direction, hitInfo.hitNormal), RandomInHemisphere(hitInfo.hitNormal, state),
						FresnelReflectAmount(1.0003, hitInfo.hitMaterial.ior, hitInfo.hitNormal, ray.direction, mix(hitInfo.hitMaterial.roughness, 0, isSpecularBounce))
					),
					mix (
						refract(ray.direction, hitInfo.hitNormal, hitInfo.hitMaterial.ior), -RandomInHemisphere(hitInfo.hitNormal, state),
						hitInfo.hitMaterial.roughness
					),
					hitInfo.hitMaterial.refractionAmount
				)
			);

			ray.origin += ray.direction * 0.0001;

			emittedLight += hitInfo.hitMaterial.emissionColor * hitInfo.hitMaterial.emissionStrength;
			incomingLight += emittedLight * rayColor;
			rayColor *= mix(hitInfo.hitMaterial.baseColor, hitInfo.hitMaterial.specularColor, isSpecularBounce);
		} else {
			currBounces++;
			
			emittedLight += skyColor * skyIntensity;
			incomingLight += emittedLight * rayColor;
			rayColor *= skyColor;
			break;
		}
	}

	return incomingLight.rgb / currBounces;
}

void main() {
	uint pixelIndex = uint(viewport.x / textureSize(accumTexture, 0).x * 4294967295.0 + viewport.y / textureSize(accumTexture, 0).y * 4294967295.0);
	uint rngState = uint(pixelIndex * 23423 * frameTime + RandomValue(pixelIndex));

	Ray ray;
	ray.origin = cam.position;

	vec3 camForward = vec3(cam.position.xy, cam.position.z - 1.0);

	ray.direction = vec3(vec2(viewportCenter.x * textureSize(accumTexture, 0).x/textureSize(accumTexture, 0).y, viewportCenter.y) + camForward.xy, camForward.z) - ray.origin;

	vec3 finalColor = vec3(0);
	vec3 rayTraceColor = vec3(0);

	int maxBounces = 12;
	rayTraceColor = RayTrace(ray, maxBounces, rngState);


	float weight = 1.0 / (currAccumPass + 1);
	finalColor = texture(accumTexture, accumTexCoords).rgb * (1 - weight) + rayTraceColor * weight;

	fragColor = vec4(finalColor, 1.0);
}