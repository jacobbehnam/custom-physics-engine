#pragma once

// Units:
//   specificHeat  _ J / (kg.K)
//   conductivity  _ W / (m.K)
//   emissivity    _ dimensionless, 0.0–1.0
//   density       _ kg / m^3
//   meltingPoint  _ Kelvin  (0 = does not apply)

struct Material {
    const char* name;
    float specificHeat;
    float conductivity;
    float emissivity;
    float density;
    float meltingPoint;
};

#define DEF_MATERIAL(name, spec_heat_j_kgK, cond_w_mK, emissivity_0_1, density_kg_m3, melt_pt_K) \
    constexpr Material name = { #name, spec_heat_j_kgK, cond_w_mK, emissivity_0_1, density_kg_m3, melt_pt_K }

namespace Materials {
    //              name        sh      cond    emis   dens   melt
    DEF_MATERIAL( Iron,        449,    79.0f,  0.70f, 7874,  1811 );
    DEF_MATERIAL( Copper,      385,   401.0f,  0.03f, 8960,  1358 );
    DEF_MATERIAL( Aluminium,   897,   237.0f,  0.05f, 2700,   933 );
    DEF_MATERIAL( Steel,       490,    50.0f,  0.80f, 7850,  1700 );
    DEF_MATERIAL( Concrete,    880,     1.7f,  0.90f, 2300,  1700 );
    DEF_MATERIAL( Glass,       840,     1.0f,  0.92f, 2500,  1400 );
    DEF_MATERIAL( Wood,       1700,     0.13f, 0.90f,  600,     0 );
    DEF_MATERIAL( Rubber,     1000,     0.16f, 0.90f, 1100,     0 );
}
