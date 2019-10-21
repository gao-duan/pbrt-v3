#if defined(_MSC_VER)
#define NOMINMAX
#pragma once
#endif

#ifndef PBRT_MATERIALS_DISNEYSIMPLE_H
#define PBRT_MATERIALS_DISNEYSIMPLE_H

// materials/disney.h*
#include "material.h"
#include "pbrt.h"

namespace pbrt {

class DisneySimpleMaterial : public Material {
  public:
    //  Public Methods
    DisneySimpleMaterial(
        const std::shared_ptr<Texture<Spectrum>> &diffuse,
        const std::shared_ptr<Texture<Spectrum>> &specular,
        const std::shared_ptr<Texture<Spectrum>> &normal,
                  
        const std::shared_ptr<Texture<Float>> &roughness,
         const std::shared_ptr<Texture<Float>> &eta)
        : diffuse(diffuse), specular(specular), normal(normal), eta(eta), roughness(roughness){}
    void ComputeScatteringFunctions(SurfaceInteraction *si, MemoryArena &arena,
                                    TransportMode mode,
                                    bool allowMultipleLobes) const;

  private:
    //  Private Data
    std::shared_ptr<Texture<Spectrum>> diffuse, specular, normal;
    std::shared_ptr<Texture<Float>> eta;
    std::shared_ptr<Texture<Float>> roughness;
};

DisneySimpleMaterial *CreateDisneySimpleMaterial(const TextureParams &mp);

}  // namespace pbrt

#endif  // PBRT_MATERIALS_DISNEY_H
