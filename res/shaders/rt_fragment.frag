#version 460

#define SCREEN_W 1920
#define SCREEN_H 1080

#define PI 3.14159265
#define TWO_PI 6.28318530
#define INFINITY 10000000.0

// Random distribution functions: https://github.com/SebLague/Ray-Tracing/blob/main/Assets/Scripts/Shaders/RayTracing.shader

in vec2 accumTexCoords;
out vec4 fragColor;

vec2 viewport = vec2(gl_FragCoord.xy / vec2(SCREEN_W, SCREEN_H));
vec2 viewportCenter = viewport - 0.5;

uniform float frameTime;
uniform float currAccumPass;

uniform sampler2D accumTexture;

struct Ray { vec3 origin; vec3 direction; };
struct Camera { vec3 position; vec3 up; vec3 right; vec3 forward; };
struct Material { vec3 baseColor; float roughness; vec3 emissionColor; float emissionStrength; float ior; float refractionAmount; bool isRefractive; bool isLight; };
struct Sphere { vec3 position; float radius; Material material; };
struct Triangle { vec3 p1; vec3 p2; vec3 p3; Material material; };
struct HitInfo { vec3 hitPoint; vec3 hitNormal; float hitDist; bool hasHit; Material hitMaterial; };

layout (std430, binding = 1) buffer vertexSSBO {
	vec4 vertices[];
};
layout (std430, binding = 2) buffer indexSSBO {
	ivec4 indices[];
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

vec3 RandomDirection(vec3 normal, inout uint state) {
	float x = RandomValueND(state);
	float y = RandomValueND(state);
	float z = RandomValueND(state);
	return normalize(vec3(x, y, z) + normal);
}

vec3 RandomInHemisphere(vec3 normal, inout uint state) {
	vec3 randomInSphere = RandomDirection(normal, state);
	if (dot(randomInSphere, normal) >= 0.0) {
		return randomInSphere;
	} else {
		return -randomInSphere;
	}
}

vec3 Vec3At(Ray ray, float t) {
	return ray.origin + t * ray.direction;
}

float LengthSquared(vec3 vec) {
	return (vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}



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

	tempHitInfo.hasHit = t > 0.00001 && !(det < 0.00001) && !(u < 0 || u > 1) && !(v < 0 || u + v > 1);
	tempHitInfo.hitPoint = Vec3At(ray, t);
	tempHitInfo.hitDist = t;
	tempHitInfo.hitNormal = normalize(cross(edge1, edge2));
	return tempHitInfo;
}

HitInfo CalculateRay(in Ray ray) {
	HitInfo closestHit;
	closestHit.hitDist = INFINITY;

	for (int i = 0; i < indices.length(); i++) {
		Triangle tempTri;

		tempTri.p1 = vertices[indices[i].x].xyz;
		tempTri.p2 = vertices[indices[i].y].xyz;
		tempTri.p3 = vertices[indices[i].z].xyz;

		tempTri.material = sphere1.material;

		HitInfo tempHit = HitTriangle(tempTri, ray);
		tempHit.hitMaterial = tempTri.material;

		if (tempHit.hasHit && tempHit.hitDist < closestHit.hitDist) {
			closestHit = tempHit;
			closestHit.hitMaterial = tempHit.hitMaterial;
		}
	}

	for (int i = 0; i < tris.length(); i++) {
		HitInfo tempHit = HitTriangle(tris[i], ray);
		tempHit.hitMaterial = tris[i].material;

		if (tempHit.hasHit && tempHit.hitDist < closestHit.hitDist) {
			closestHit = tempHit;
			closestHit.hitMaterial = tempHit.hitMaterial;
		}
	}

	for (int i = 0; i < spheres.length(); i++) {
		HitInfo tempHit = HitSphere(spheres[i].position, spheres[i].radius, ray);
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

	for (int i = 0; i < maxBounces; i++) {
		HitInfo hitInfo = CalculateRay(ray);
		if (hitInfo.hasHit && hitInfo.hitDist < INFINITY) {
			currBounces++;

			ray.origin = hitInfo.hitPoint;

			float test = max(rayColor.r, max(rayColor.g, rayColor.b));
			if (RandomValue(state) > test) {
				break;
			}

			if (hitInfo.hitMaterial.isRefractive) {
				if (RandomValue(state) < hitInfo.hitMaterial.refractionAmount) {
					ray.direction = normalize(refract(ray.direction, hitInfo.hitNormal, hitInfo.hitMaterial.ior) - RandomInHemisphere(hitInfo.hitNormal, state) * hitInfo.hitMaterial.roughness);
				} else {
					ray.direction = normalize(mix(reflect(ray.direction, hitInfo.hitNormal), RandomInHemisphere(hitInfo.hitNormal, state), hitInfo.hitMaterial.roughness));
				}
			} else if (hitInfo.hitMaterial.isLight) {
				ray.origin = hitInfo.hitPoint + ray.direction * 0.001;
				continue;
			} else {
				ray.direction = normalize(mix(reflect(ray.direction, hitInfo.hitNormal), RandomInHemisphere(hitInfo.hitNormal, state), hitInfo.hitMaterial.roughness));
			}

			emittedLight = hitInfo.hitMaterial.emissionColor * hitInfo.hitMaterial.emissionStrength;
			rayColor *= hitInfo.hitMaterial.baseColor;
			incomingLight += emittedLight * rayColor;
		} else {
			currBounces++;
			
			emittedLight = skyColor * skyIntensity;
			rayColor *= emittedLight;
			incomingLight += rayColor;
			break;
		}
	}
	return incomingLight / currBounces;
}

vec4 DownSampleFrame(sampler2D uniformSampler, float offset)
{
    vec2 pixelOffset = vec2(1.0f/textureSize(uniformSampler, 0).x, 1.0f/textureSize(uniformSampler, 0).y);

    vec3 downScaleColor = vec3(0.0f);
    {
        float Pixels = offset;
        float dx = (1.0 / Pixels);
        float dy = (textureSize(uniformSampler, 0).x / textureSize(uniformSampler, 0).y / Pixels);
        vec2 Coord = vec2(dx * floor(accumTexCoords.s / dx),
                          dy * floor(accumTexCoords.t / dy));

        downScaleColor += texture(uniformSampler, vec2(Coord.x - -pixelOffset.x, Coord.y)).xyz;
        downScaleColor += texture(uniformSampler, vec2(Coord.x + -pixelOffset.x, Coord.y)).xyz;
        downScaleColor += texture(uniformSampler, vec2(Coord.x, Coord.y - pixelOffset.y)).xyz;
        downScaleColor += texture(uniformSampler, vec2(Coord.x, Coord.y + pixelOffset.y)).xyz;
	    downScaleColor *= 0.25f;
    }
    return (vec4(downScaleColor, 1.0f));
}

void main() {
	uint pixelIndex = uint(viewport.x / SCREEN_W * 4294967295.0 + viewport.y / SCREEN_H * 4294967295.0);
	uint rngState = uint(pixelIndex * 719393 * frameTime);

	Ray ray;
	ray.origin = cam.position;
	ray.direction = vec3(vec2(viewportCenter.x * 16/9, viewportCenter.y) + cam.forward.xy, cam.forward.z) - ray.origin;

	vec3 accumColor = vec3(0);
	vec3 rayTraceColor = vec3(0);

	int maxBounces = 18;
	int spp = 6;
	for (int i = 0; i < spp; ++i) {
		rayTraceColor += RayTrace(ray, maxBounces, rngState);
	}
	rayTraceColor /= spp;


	float weight = 1.0 / (currAccumPass + 1);
	accumColor = texture(accumTexture, accumTexCoords).xyz * (1 - weight) + rayTraceColor * weight;

	fragColor = vec4(accumColor, 1.0);
}