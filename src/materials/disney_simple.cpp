#include "bssrdf.h"
#include "interaction.h"
#include "materials/disney_simple.h"
#include "paramset.h"
#include "reflection.h"
#include "rng.h"
#include "stats.h"
#include "stringprint.h"
#include "texture.h"

namespace pbrt {

static Float sqr(Float x) { return x * x; }
static Float SchlickWeight(Float cosTheta) {
    Float m = Clamp(1 - cosTheta, 0, 1);
    return (m * m) * (m * m) * m;
}

static Float FrSchlick(Float R0, Float cosTheta) {
    return Lerp(SchlickWeight(cosTheta), R0, 1);
}

static Spectrum FrSchlick(const Spectrum &R0, Float cosTheta) {
    return Lerp(SchlickWeight(cosTheta), R0, Spectrum(1.));
}
static Float SchlickR0FromEta(Float eta) { return sqr(eta - 1) / sqr(eta + 1); }


namespace Wrapper {
class DisneyDiffuse : public BxDF {
  public:
    DisneyDiffuse(const Spectrum &R)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), R(R) {}
    Spectrum f(const Vector3f &wo, const Vector3f &wi) const {
        Float Fo = SchlickWeight(AbsCosTheta(wo)),
              Fi = SchlickWeight(AbsCosTheta(wi));

        // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing.
        // Burley 2015, eq (4).
        return R * InvPi * (1 - Fo / 2) * (1 - Fi / 2);
    }
    Spectrum rho(const Vector3f &, int, const Point2f *) const { return R; }
    Spectrum rho(int, const Point2f *, const Point2f *) const { return R; }
    std::string ToString() const {
        return StringPrintf("[ DisneyDiffuse R: %s ]", R.ToString().c_str());
    }

  private:
    Spectrum R;
};

class DisneyRetro : public BxDF {
  public:
    DisneyRetro(const Spectrum &R, Float roughness)
        : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)),
          R(R),
          roughness(roughness) {}
    Spectrum f(const Vector3f &wo, const Vector3f &wi) const {
        Vector3f wh = wi + wo;
        if (wh.x == 0 && wh.y == 0 && wh.z == 0) return Spectrum(0.);
        wh = Normalize(wh);
        Float cosThetaD = Dot(wi, wh);

        Float Fo = SchlickWeight(AbsCosTheta(wo)),
              Fi = SchlickWeight(AbsCosTheta(wi));
        Float Rr = 2 * roughness * cosThetaD * cosThetaD;

        // Burley 2015, eq (4).
        return R * InvPi * Rr * (Fo + Fi + Fo * Fi * (Rr - 1));
    }
    Spectrum rho(const Vector3f &, int, const Point2f *) const { return R; }
    Spectrum rho(int, const Point2f *, const Point2f *) const { return R; }
    std::string ToString() const {
        return StringPrintf("[ DisneyRetro R: %s roughness: %f ]",
                            R.ToString().c_str(), roughness);
    }

  private:
    Spectrum R;
    Float roughness;
};

class DisneyMicrofacetDistribution : public TrowbridgeReitzDistribution {
  public:
    DisneyMicrofacetDistribution(Float alphax, Float alphay)
        : TrowbridgeReitzDistribution(alphax, alphay) {}

    Float G(const Vector3f &wo, const Vector3f &wi) const {
        // Disney uses the separable masking-shadowing model.
        return G1(wo) * G1(wi);
    }
};

class DisneyFresnel : public Fresnel {
  public:
    DisneyFresnel(const Spectrum &R0, Float metallic, Float eta)
        : R0(R0), metallic(metallic), eta(eta) {}
    Spectrum Evaluate(Float cosI) const {
        return Lerp(metallic, Spectrum(FrDielectric(cosI, 1, eta)),
                    FrSchlick(R0, cosI));
    }
    std::string ToString() const {
        return StringPrintf("[ DisneyFresnel R0: %s metallic: %f eta: %f ]",
                            R0.ToString().c_str(), metallic, eta);
    }

  private:
    const Spectrum R0;
    const Float metallic, eta;
};
}  // namespace Wrapper

void DisneySimpleMaterial::ComputeScatteringFunctions(
    SurfaceInteraction *si,MemoryArena &arena,TransportMode mode,bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (normal) NormalMap(normal, si);

    // Evaluate textures for _DisneyMaterial_ material and allocate BRDF
    si->bsdf = ARENA_ALLOC(arena, BSDF)(*si);

    // Diffuse
    Spectrum d = diffuse->Evaluate(*si).Clamp();
    Spectrum s = specular->Evaluate(*si).Clamp();


    Spectrum c = d + s;
    Float lum = c.y();
    Spectrum Ctint = lum > 0 ? (c / lum) : Spectrum(1.); // normalize lum. to isolate hue+sat


    Float metallicWeight = lum > 0 ? s.y() / lum : 0.0f;
    Float e = eta->Evaluate(*si);
    Float diffuseWeight = (1 - metallicWeight);
   
    Float rough = roughness->Evaluate(*si);
    

    if (diffuseWeight > 0) {  
        // No subsurface scattering; use regular (Fresnel modified)
        // diffuse.
        si->bsdf->Add(
            ARENA_ALLOC(arena, Wrapper::DisneyDiffuse)(d));
            
        // Retro-reflection.
        si->bsdf->Add(
            ARENA_ALLOC(arena, Wrapper::DisneyRetro)(d, rough));
    }

    // Create the microfacet distribution for metallic and/or specular
    // transmission.
    Float aspect = 1.0f;
    Float ax = std::max(Float(.001), sqr(rough) / aspect);
    Float ay = std::max(Float(.001), sqr(rough) * aspect);
    MicrofacetDistribution *distrib =
        ARENA_ALLOC(arena, Wrapper::DisneyMicrofacetDistribution)(ax, ay);

    // Specular is Trowbridge-Reitz with a modified Fresnel function.
    Float specTint = 0;
    Spectrum Cspec0 =
        Lerp(metallicWeight,
             SchlickR0FromEta(e) * Lerp(specTint, Spectrum(1.), Ctint), c);
    Fresnel *fresnel =
        ARENA_ALLOC(arena, Wrapper::DisneyFresnel)(Cspec0, metallicWeight, e);
    si->bsdf->Add(ARENA_ALLOC(arena, MicrofacetReflection)(Spectrum(1.),
                                                           distrib, fresnel));

}

DisneySimpleMaterial *CreateDisneySimpleMaterial(const TextureParams &mp) {
    std::shared_ptr<Texture<Spectrum>> diffuse =
        mp.GetSpectrumTexture("diffuse", Spectrum(1.0f));
    std::shared_ptr<Texture<Spectrum>> specular =
        mp.GetSpectrumTexture("specular", Spectrum(0.0f));

    std::shared_ptr<Texture<Float>> eta = mp.GetFloatTexture("eta", 1.5f);
    std::shared_ptr<Texture<Float>> roughness =
        mp.GetFloatTexture("roughness", .5f);
    std::shared_ptr<Texture<Spectrum>> normal = mp.GetSpectrumTextureOrNull("normal");
    return new DisneySimpleMaterial(diffuse, specular, normal, roughness, eta);
}

}  // namespace pbrt