#include "integrators/field.h"
#include "sampling.h"
#include "interaction.h"
#include "paramset.h"
#include "camera.h"
#include "film.h"
#include "scene.h"

namespace pbrt {
pbrt::FieldIntegrator::FieldIntegrator(const std::string& field,
                                       const Spectrum& undefined,
                                       std::shared_ptr<const Camera> camera,
                                       std::shared_ptr<Sampler> sampler,
                                       const Bounds2i& pixelBounds)
    :SamplerIntegrator(camera, sampler, pixelBounds), undefined(undefined) {
		if (field == "position") {
			type = EPosition;
		} else if (field == "relPosition") {
			type = ERelativePosition;
		} else if (field == "distance") {
                    type = EDistance;
		} else if (field == "geoNormal") {
                    type = EGeometricNormal;
		} else if (field == "shNormal") {
                    type = EShadingNormal;
		} else if (field == "uv") {
                    type = EUV;
		} else if (field == "albedo") {
                    type = EAlbedo;
		} else if (field == "mask") {
                    type = EMask;
		}
}

Spectrum FieldIntegrator::Li(
    const RayDifferential& ray, const Scene& scene,
                             Sampler& sampler, MemoryArena& arena,
                             int depth) const {
    Spectrum result(undefined);
   
	SurfaceInteraction isect;
    if (!scene.Intersect(ray, &isect)) {
            return result;
	}

	Float rgb[3];
        switch (type) {
        case EPosition:
            rgb[0] = isect.p.x;
            rgb[1] = isect.p.y;
            rgb[2] = isect.p.z;
            break;
        case ERelativePosition: 
			/*{
                const Transform& t = ;
                    camera->getWorldTransform()->eval(its.t).inverse();
                auto t = (camera->CameraToWorld);
                Point p = (its.p);

                            rgb[0] = isect.p.x;
                rgb[1] = isect.p.y;
                rgb[2] = isect.p.z;
                result.fromLinearRGB(p.x, p.y, p.z);
            } */
            break;
        case EDistance:
            rgb[0] = ray.tMax;
            rgb[1] = rgb[0];
            rgb[2] = rgb[0];
            break;
        case EGeometricNormal:
            rgb[0] = isect.n.x * 0.5 + 0.5;
            rgb[1] = isect.n.y * 0.5 + 0.5;
            rgb[2] = isect.n.z * 0.5 + 0.5;
            break;
        case EShadingNormal:
            rgb[0] = isect.shading.n.x * 0.5 + 0.5;
            rgb[1] = isect.shading.n.y * 0.5 + 0.5;
            rgb[2] = isect.shading.n.z * 0.5 + 0.5;
            break;
        case EUV:
            rgb[0] = isect.uv.x;
            rgb[1] = isect.uv.y;
            rgb[2] = 0;
            break;
        case EAlbedo:
            /*isect.Get
result = its.shape->getBSDF()->getDiffuseReflectance(its);*/
            break;

        case EMask:
            rgb[0] = 1;
            rgb[1] = rgb[0];
            rgb[2] = rgb[0];
            break;
        default:
            break;
	}
	result = RGBSpectrum::FromRGB(rgb);
   // result = Clamp(result, 0, 1);
	return result;
}

FieldIntegrator* CreateFieldIntegrator(const ParamSet& params,
                                       std::shared_ptr<Sampler> sampler,
                                       std::shared_ptr<const Camera> camera) {
    int np;
    const int* pb = params.FindInt("pixelbounds", &np);
    Bounds2i pixelBounds = camera->film->GetSampleBounds();
    if (pb) {
        if (np != 4)
            Error("Expected four values for \"pixelbounds\" parameter. Got %d.",
                  np);
        else {
            pixelBounds = Intersect(pixelBounds,
                                    Bounds2i{{pb[0], pb[2]}, {pb[1], pb[3]}});
            if (pixelBounds.Area() == 0)
                Error("Degenerate \"pixelbounds\" specified.");
        }
    }
    std::string field = params.FindOneString("field", "mask");
    Spectrum default = params.FindOneSpectrum("default", 0);

    return new FieldIntegrator(field, default, camera, sampler,
                               pixelBounds);
}

}  // namespace pbrt