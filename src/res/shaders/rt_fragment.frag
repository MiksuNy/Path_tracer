#version 460

#define SCREEN_W 1920
#define SCREEN_H 1080

#define PI 3.14159265
#define TWO_PI 6.28318530
#define INFINITY 1000000.0

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
struct HitInfo { vec3 hitPoint; vec3 hitNormal; float hitDist; bool hasHit; Material hitMaterial; };

uniform Camera cam;

uniform Sphere sphere1;
uniform Sphere sphere2;
uniform Sphere sphere3;
uniform Sphere ground;
uniform Sphere light;

Sphere spheres[] = {
	sphere1, sphere2, sphere3, ground, light
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
	float x = RandomValueND(state) + normal.x;
	float y = RandomValueND(state) + normal.y;
	float z = RandomValueND(state) + normal.z;
	return vec3(x, y, z);
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

    if (t >= 0.00005) {
		tempHitInfo.hasHit = true;
		tempHitInfo.hitPoint = ray.origin + ray.direction * t;
		tempHitInfo.hitDist = t;
		tempHitInfo.hitNormal = normalize(tempHitInfo.hitPoint - center);
		return tempHitInfo;
	} else {
		tempHitInfo.hasHit = false;
		return tempHitInfo;
	}
}

HitInfo CalculateRay(Ray ray) {
	HitInfo closestHit;
	closestHit.hitDist = INFINITY;

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

vec3 RayTrace(Ray ray, int maxBounces, inout uint state) {
	vec3 rayColor = vec3(1);
	vec3 incomingLight = vec3(0);
	vec3 emittedLight = vec3(0);

	vec3 skyLight = vec3(0.96, 0.9, 1.0);
	float skyIntensity = 0.0;

	int currBounces = 0;

	for (int i = 0; i < maxBounces; i++) {
		HitInfo hitInfo = CalculateRay(ray);
		if (hitInfo.hasHit && hitInfo.hitDist < INFINITY) {
			currBounces++;

			ray.origin = hitInfo.hitPoint;

			if (hitInfo.hitMaterial.isRefractive) {
				if (RandomValue(state) < hitInfo.hitMaterial.refractionAmount) {
					ray.direction = refract(ray.direction, hitInfo.hitNormal, hitInfo.hitMaterial.ior) - RandomInHemisphere(hitInfo.hitNormal, state) * hitInfo.hitMaterial.roughness;
					if (hitInfo.hasHit) {
						ray.direction = refract(ray.direction, hitInfo.hitNormal, hitInfo.hitMaterial.ior) - RandomInHemisphere(hitInfo.hitNormal, state) * hitInfo.hitMaterial.roughness;
					}
				} else {
					ray.direction = reflect(ray.direction, hitInfo.hitNormal) + RandomInHemisphere(hitInfo.hitNormal, state) * hitInfo.hitMaterial.roughness;
				}
			} else if (hitInfo.hitMaterial.isLight) {
				break;
			} else {
				ray.direction = reflect(ray.direction, hitInfo.hitNormal) + RandomInHemisphere(hitInfo.hitNormal, state) * hitInfo.hitMaterial.roughness;
			}

			emittedLight += hitInfo.hitMaterial.emissionColor * hitInfo.hitMaterial.emissionStrength;
			rayColor *= hitInfo.hitMaterial.baseColor / currAccumPass;
			incomingLight += emittedLight * rayColor;
		} else {
			currBounces++;
			
			emittedLight += skyLight * skyIntensity;
			rayColor *= emittedLight / currAccumPass;
			incomingLight += rayColor;
			break;
		}
	}
	return incomingLight /= currBounces;
}

void main() {
	uint pixelIndex = uint(viewport.x / SCREEN_W * 4294967295.0 + viewport.y / SCREEN_H * 4294967295.0 * currAccumPass);
	uint rngState = uint(pixelIndex * 719393 * frameTime / currAccumPass);

	Ray ray;
	ray.origin = cam.position;
	ray.direction = vec3(vec2(viewportCenter.x * 16/9, viewportCenter.y) + cam.position.xy, cam.forward) - ray.origin;

	vec3 accumColor = texture(accumTexture, accumTexCoords).xyz;
	vec3 rayTraceColor = vec3(0);

	int maxBounces = 16;
	int samples = 50;
	for (int i = 0; i < samples; i++) {
		rayTraceColor += RayTrace(ray, maxBounces, rngState);
	}
	rayTraceColor /= samples;

	fragColor = vec4((rayTraceColor / currAccumPass) + accumColor, 1.0);
}