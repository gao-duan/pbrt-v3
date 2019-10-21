#if defined(_MSC_VER)
#define NOMINMAX
#pragma once
#endif

#ifndef PBRT_INTEGRATORS_FIELD_H
#define PBRT_INTEGRATORS_FIELD_H

// integrators/ao.h*
#include "pbrt.h"
#include "integrator.h"


namespace pbrt {

// AOIntegrator Declarations
class FieldIntegrator : public SamplerIntegrator {
  public:
    enum EField {
        EPosition,
        ERelativePosition,
        EDistance,
        EGeometricNormal,
        EShadingNormal,
        EUV,
        EAlbedo,
        EMask
    };

    // AOIntegrator Public Methods
    FieldIntegrator(const std::string& type, const Spectrum& undefined,
                 std::shared_ptr<const Camera> camera,
                 std::shared_ptr<Sampler> sampler, const Bounds2i &pixelBounds);
    Spectrum Li(const RayDifferential &ray, const Scene &scene,
                Sampler &sampler, MemoryArena &arena, int depth) const;

  private:
    EField type;
    Spectrum undefined;
};

FieldIntegrator *CreateFieldIntegrator(const ParamSet &params,
                                 std::shared_ptr<Sampler> sampler,
                                 std::shared_ptr<const Camera> camera);

}  // namespace pbrt

#endif  // PBRT_INTEGRATORS_PATH_H
