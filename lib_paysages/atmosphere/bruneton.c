#include "public.h"

/*
 * Atmospheric scattering, based on E. Bruneton and F.Neyret work.
 * http://evasion.inrialpes.fr/~Eric.Bruneton/
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../system.h"
#include "../tools.h"
#include "../tools/cache.h"
#include "../tools/texture.h"
#include "../tools/parallel.h"
#include "../renderer.h"

/*********************** Constants ***********************/

#define WORLD_SCALING 0.05
#define GROUND_OFFSET 10.0
static const double Rg = 6360.0;
static const double Rt = 6420.0;
static const double RL = 6421.0;
static const double exposure = 0.4;
static const double ISun = 100.0;
static const double AVERAGE_GROUND_REFLECTANCE = 0.1;

#if 0
#define RES_MU 128
#define RES_MU_S 32
#define RES_R 32
#define RES_NU 8
#define SKY_W 64
#define SKY_H 16
#define TRANSMITTANCE_W 256
#define TRANSMITTANCE_H 64
#define TRANSMITTANCE_INTEGRAL_SAMPLES 500
#define INSCATTER_INTEGRAL_SAMPLES 50
#define IRRADIANCE_INTEGRAL_SAMPLES 32
#define INSCATTER_SPHERICAL_INTEGRAL_SAMPLES 16
#else
#define RES_MU 64
#define RES_MU_S 16
#define RES_R 16
#define RES_NU 8
#define SKY_W 64
#define SKY_H 16
#define TRANSMITTANCE_W 256
#define TRANSMITTANCE_H 64
#define TRANSMITTANCE_INTEGRAL_SAMPLES 100
#define INSCATTER_INTEGRAL_SAMPLES 10
#define IRRADIANCE_INTEGRAL_SAMPLES 16
#define INSCATTER_SPHERICAL_INTEGRAL_SAMPLES 8
#endif

Texture2D* _transmittanceTexture = NULL;
Texture2D* _irradianceTexture = NULL;
Texture3D* _inscatterTexture = NULL;

/* Rayleigh */
static const double HR = 8.0;
static const Color betaR = {5.8e-3, 1.35e-2, 3.31e-2, 1.0};

/* Mie */
/* DEFAULT */
static const double HM = 1.2;
static const Vector3 betaMSca = {4e-3, 4e-3, 4e-3};
static const Vector3 betaMEx = {4e-3 / 0.9, 4e-3 / 0.9, 4e-3 / 0.9};
static const double mieG = 0.8;
/* CLEAR SKY */
/*static const double HM = 1.2;
static const Vector3 betaMSca = {20e-3, 20e-3, 20e-3};
static const Vector3 betaMEx = {20e-3 / 0.9, 20e-3 / 0.9, 20e-3 / 0.9};
static const double mieG = 0.76;*/
/* PARTLY CLOUDY */
/*static const double HM = 3.0;
static const Vector3 betaMSca = {3e-3, 3e-3, 3e-3};
static const Vector3 betaMEx = {3e-3 / 0.9, 3e-3 / 0.9, 3e-3 / 0.9};
static const double mieG = 0.65;*/

/*********************** Shader helpers ***********************/

#define step(_a_,_b_) ((_b_) < (_a_) ? 0.0 : 1.0)
#define sign(_a_) ((_a_) < 0.0 ? -1.0 : ((_a_) > 0.0 ? 1.0 : 0.0))
#define mix(_x_,_y_,_a_) ((_x_) * (1.0 - (_a_)) + (_y_) * (_a_))
static inline double min(double a, double b)
{
    return a < b ? a : b;
}
static inline double max(double a, double b)
{
    return a > b ? a : b;
}
static inline Color vec4mix(Color v1, Color v2, double a)
{
    v1.r = mix(v1.r, v2.r, a);
    v1.g = mix(v1.g, v2.g, a);
    v1.b = mix(v1.b, v2.b, a);
    v1.a = mix(v1.a, v2.a, a);
    return v1;
}
static inline double clamp(double x, double minVal, double maxVal)
{
    if (x < minVal)
    {
        x = minVal;
    }
    return (x > maxVal) ? maxVal : x;
}
static inline double smoothstep(double edge0, double edge1, double x)
{
    double t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}
static inline void _fixVec4Min(Color* vec, double minVal)
{
    if (vec->r < minVal) { vec->r = minVal; }
    if (vec->g < minVal) { vec->g = minVal; }
    if (vec->b < minVal) { vec->b = minVal; }
    if (vec->a < minVal) { vec->a = minVal; }
}
static inline Color vec4max(Color vec, double minVal)
{
    if (vec.r < minVal) { vec.r = minVal; }
    if (vec.g < minVal) { vec.g = minVal; }
    if (vec.b < minVal) { vec.b = minVal; }
    if (vec.a < minVal) { vec.a = minVal; }
    return vec;
}

static inline Vector3 vec3(double x, double y, double z)
{
    Vector3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

static inline Color vec4(double r, double g, double b, double a)
{
    Color result;
    result.r = r;
    result.g = g;
    result.b = b;
    result.a = a;
    return result;
}

/*********************** Texture manipulation ***********************/

static inline Color _texture3D(Texture3D* tex, Vector3 p)
{
    return texture3DGetLinear(tex, p.x, p.y, p.z);
}

static Color _texture4D(Texture3D* tex3d, double r, double mu, double muS, double nu)
{
    if (r < Rg + 0.00000001) r = Rg + 0.00000001;
    double H = sqrt(Rt * Rt - Rg * Rg);
    double rho = sqrt(r * r - Rg * Rg);
    double rmu = r * mu;
    double delta = rmu * rmu - r * r + Rg * Rg;
    Color cst = (rmu < 0.0 && delta > 0.0) ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / (double)(RES_MU)) : vec4(-1.0, H * H, H, 0.5 + 0.5 / (double)(RES_MU));
    double uR = 0.5 / (double)(RES_R) + rho / H * (1.0 - 1.0 / (double)(RES_R));
    double uMu = cst.a + (rmu * cst.r + sqrt(delta + cst.g)) / (rho + cst.b) * (0.5 - 1.0 / (double)(RES_MU));
    double uMuS = 0.5 / (double)(RES_MU_S) + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / (double)(RES_MU_S));
    double lerp = (nu + 1.0) / 2.0 * ((double)(RES_NU) - 1.0);
    double uNu = floor(lerp);
    lerp = lerp - uNu;
    return vec4mix(_texture3D(tex3d, vec3((uNu + uMuS) / (double)(RES_NU), uMu, uR)), _texture3D(tex3d, vec3((uNu + uMuS + 1.0) / (double)(RES_NU), uMu, uR)), lerp);
}

/*********************** Physics functions ***********************/

/* Rayleigh phase function */
static double _phaseFunctionR(double mu)
{
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}

/* Mie phase function */
static double _phaseFunctionM(double mu)
{
    return 1.5 * 1.0 / (4.0 * M_PI) * (1.0 - mieG * mieG) * pow(1.0 + (mieG * mieG) - 2.0 * mieG * mu, -3.0 / 2.0) * (1.0 + mu * mu) / (2.0 + mieG * mieG);
}

/* approximated single Mie scattering (cf. approximate Cm in paragraph "Angular precision") */
static Color _getMie(Color rayMie)
{
    Color result;

    result.r = rayMie.r * rayMie.a / max(rayMie.r, 1e-4) * (betaR.r / betaR.r);
    result.g = rayMie.g * rayMie.a / max(rayMie.r, 1e-4) * (betaR.r / betaR.g);
    result.b = rayMie.b * rayMie.a / max(rayMie.r, 1e-4) * (betaR.r / betaR.b);
    result.a = 1.0;

    return result;
}

/* optical depth for ray (r,mu) of length d, using analytic formula
   (mu=cos(view zenith angle)), intersections with ground ignored
   H=height scale of exponential density function */
static double _opticalDepth(double H, double r, double mu, double d)
{
    double a = sqrt((0.5 / H) * r);
    double ax = a * (mu);
    double ay = a * (mu + d / r);
    double axs = sign(ax);
    double ays = sign(ay);
    double axq = ax * ax;
    double ayq = ay * ay;
    double x = ays > axs ? exp(axq) : 0.0;
    double yx = axs / (2.3193 * fabs(ax) + sqrt(1.52 * axq + 4.0));
    double yy = ays / (2.3193 * fabs(ay) + sqrt(1.52 * ayq + 4.0)) * exp(-d / H * (d / (2.0 * r) + mu));
    return sqrt((6.2831 * H) * r) * exp((Rg - r) / H) * (x + yx - yy);
}

static inline void _getTransmittanceUV(double r, double mu, double* u, double* v)
{
    if (r < Rg + 0.00000001) r = Rg + 0.00000001;
    double dr = (r - Rg) / (Rt - Rg);
    *v = sqrt(dr);
    *u = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5;
}

/* transmittance(=transparency) of atmosphere for infinite ray (r,mu)
   (mu=cos(view zenith angle)), intersections with ground ignored */
static Color _transmittance(double r, double mu)
{
    double u, v;
    _getTransmittanceUV(r, mu, &u, &v);
    return texture2DGetLinear(_transmittanceTexture, u, v);
}

/* transmittance(=transparency) of atmosphere between x and x0
 * assume segment x,x0 not intersecting ground
 * d = distance between x and x0, mu=cos(zenith angle of [x,x0) ray at x) */
static Color _transmittance3(double r, double mu, double d)
{
    Color result, t1, t2;
    double r1 = sqrt(r * r + d * d + 2.0 * r * mu * d);
    double mu1 = (r * mu + d) / r1;
    if (mu > 0.0)
    {
        t1 = _transmittance(r, mu);
        t2 = _transmittance(r1, mu1);
    }
    else
    {
        t1 = _transmittance(r1, -mu1);
        t2 = _transmittance(r, -mu);
    }
    result.r = min(t1.r / t2.r, 1.0);
    result.g = min(t1.g / t2.g, 1.0);
    result.b = min(t1.b / t2.b, 1.0);
    result.a = 1.0;
    return result;
}

static void _getIrradianceRMuS(double x, double y, double* r, double* muS)
{
    *r = Rg + y * (Rt - Rg);
    *muS = -0.2 + x * (1.0 + 0.2);
}

/* nearest intersection of ray r,mu with ground or top atmosphere boundary
 * mu=cos(ray zenith angle at ray origin) */
static double _limit(double r, double mu)
{
    double dout = -r * mu + sqrt(r * r * (mu * mu - 1.0) + RL * RL);
    double delta2 = r * r * (mu * mu - 1.0) + Rg * Rg;
    if (delta2 >= 0.0)
    {
        double din = -r * mu - sqrt(delta2);
        if (din >= 0.0) {
            dout = min(dout, din);
        }
    }
    return dout;
}

/* transmittance(=transparency) of atmosphere for ray (r,mu) of length d
   (mu=cos(view zenith angle)), intersections with ground ignored
   uses analytic formula instead of transmittance texture */
static Vector3 _analyticTransmittance(double r, double mu, double d)
{
    Vector3 result;

    result.x = exp(-betaR.r * _opticalDepth(HR, r, mu, d) - betaMEx.x * _opticalDepth(HM, r, mu, d));
    result.y = exp(-betaR.g * _opticalDepth(HR, r, mu, d) - betaMEx.y * _opticalDepth(HM, r, mu, d));
    result.z = exp(-betaR.b * _opticalDepth(HR, r, mu, d) - betaMEx.z * _opticalDepth(HM, r, mu, d));

    return result;
}

/* transmittance(=transparency) of atmosphere for infinite ray (r,mu)
   (mu=cos(view zenith angle)), or zero if ray intersects ground */
static Color _transmittanceWithShadow(double r, double mu)
{
    return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? COLOR_BLACK : _transmittance(r, mu);
}

static void _getMuMuSNu(double x, double y, double r, Color dhdH, double* mu, double* muS, double* nu)
{
    double d;

    if (y < (double)(RES_MU) / 2.0)
    {
        d = 1.0 - y / ((double)(RES_MU) / 2.0);
        d = min(max(dhdH.b, d * dhdH.a), dhdH.a * 0.999);
        *mu = (Rg * Rg - r * r - d * d) / (2.0 * r * d);
        *mu = min(*mu, -sqrt(1.0 - (Rg / r) * (Rg / r)) - 0.001);
    }
    else
    {
        d = (y - (double)(RES_MU) / 2.0) / ((double)(RES_MU) / 2.0);
        d = min(max(dhdH.r, d * dhdH.g), dhdH.g * 0.999);
        *mu = (Rt * Rt - r * r - d * d) / (2.0 * r * d);
    }
    *muS = fmod(x, (double)(RES_MU_S)) / ((double)(RES_MU_S));
    *muS = tan((2.0 * (*muS) - 1.0 + 0.26) * 1.1) / tan(1.26 * 1.1);
    *nu = -1.0 + floor(x / (double)(RES_MU_S)) / ((double)(RES_NU)) * 2.0;
}

static void _getIrradianceUV(double r, double muS, double* uMuS, double* uR)
{
    *uR = (r - Rg) / (Rt - Rg);
    *uMuS = (muS + 0.2) / (1.0 + 0.2);
}

static Color _irradiance(Texture2D* sampler, double r, double muS)
{
    double u, v;
    _getIrradianceUV(r, muS, &u, &v);
    return texture2DGetLinear(sampler, u, v);
}

/*********************** transmittance.glsl ***********************/

static void _getTransmittanceRMu(double x, double y, double* r, double* muS)
{
    *r = Rg + (y * y) * (Rt - Rg);
    *muS = -0.15 + tan(1.5 * x) / tan(1.5) * (1.0 + 0.15);
}

static double _opticalDepthTransmittance(double H, double r, double mu)
{
    double result = 0.0;
    double dx = _limit(r, mu) / (double)TRANSMITTANCE_INTEGRAL_SAMPLES;
    double yi = exp(-(r - Rg) / H);
    int i;
    for (i = 1; i <= TRANSMITTANCE_INTEGRAL_SAMPLES; ++i) {
        double xj = (double)i * dx;
        double yj = exp(-(sqrt(r * r + xj * xj + 2.0 * xj * r * mu) - Rg) / H);
        result += (yi + yj) / 2.0 * dx;
        yi = yj;
    }
    return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? 1e9 : result;
}

static void _precomputeTransmittanceTexture()
{
    int x, y;

    for (x = 0; x < TRANSMITTANCE_W; x++)
    {
        for (y = 0; y < TRANSMITTANCE_H; y++)
        {
            double r, muS;
            _getTransmittanceRMu((double)(x + 0.5) / TRANSMITTANCE_W, (double)(y + 0.5) / TRANSMITTANCE_H, &r, &muS);
            double depth1 = _opticalDepthTransmittance(HR, r, muS);
            double depth2 = _opticalDepthTransmittance(HM, r, muS);
            Color trans;
            trans.r = exp(-(betaR.r * depth1 + betaMEx.x * depth2));
            trans.g = exp(-(betaR.g * depth1 + betaMEx.y * depth2));
            trans.b = exp(-(betaR.b * depth1 + betaMEx.z * depth2));
            trans.a = 1.0;
            texture2DSetPixel(_transmittanceTexture, x, y, trans); /* Eq (5) */
        }
    }
}

/*********************** irradiance1.glsl ***********************/

static void _precomputeIrrDeltaETexture(Texture2D* destination)
{
    int x, y;

    /* Irradiance program */
    for (x = 0; x < SKY_W; x++)
    {
        for (y = 0; y < SKY_H; y++)
        {
            double r, muS;
            Color trans, irr;
            _getIrradianceRMuS((double)x / SKY_W, (double)y / SKY_H, &r, &muS);
            trans = _transmittance(r, muS);

            irr.r = trans.r * max(muS, 0.0);
            irr.g = trans.g * max(muS, 0.0);
            irr.b = trans.b * max(muS, 0.0);
            irr.a = 1.0;

            texture2DSetPixel(destination, x, y, irr);
        }
    }
}

static void _getLayerParams(int layer, double* _r, Color* _dhdH)
{
    double r = layer / (RES_R - 1.0);
    r = r * r;
    r = sqrt(Rg * Rg + r * (Rt * Rt - Rg * Rg)) + (layer == 0 ? 0.01 : (layer == RES_R - 1 ? -0.001 : 0.0));
    double dmin = Rt - r;
    double dmax = sqrt(r * r - Rg * Rg) + sqrt(Rt * Rt - Rg * Rg);
    double dminp = r - Rg;
    double dmaxp = sqrt(r * r - Rg * Rg);

    *_r = r;
    _dhdH->r = dmin;
    _dhdH->g = dmax;
    _dhdH->b = dminp;
    _dhdH->a = dmaxp;
}

/*********************** inscatter1.glsl ***********************/

static void _integrand1(double r, double mu, double muS, double nu, double t, Color* ray, Color* mie)
{
    double ri = sqrt(r * r + t * t + 2.0 * r * mu * t);
    double muSi = (nu * t + muS * r) / ri;
    ri = max(Rg, ri);
    if (muSi >= -sqrt(1.0 - Rg * Rg / (ri * ri)))
    {
        Color t1, t2;
        t1 = _transmittance3(r, mu, t);
        t2 = _transmittance(ri, muSi);
        double fR = exp(-(ri - Rg) / HR);
        double fM = exp(-(ri - Rg) / HM);
        ray->r = fR * t1.r * t2.r;
        ray->g = fR * t1.g * t2.g;
        ray->b = fR * t1.b * t2.b;
        mie->r = fM * t1.r * t2.r;
        mie->g = fM * t1.g * t2.g;
        mie->b = fM * t1.b * t2.b;
    }
    else
    {
        ray->r = ray->g = ray->b = 0.0;
        mie->r = mie->g = mie->b = 0.0;
    }
}

static void _inscatter1(double r, double mu, double muS, double nu, Color* ray, Color* mie)
{
    ray->r = ray->g = ray->b = 0.0;
    mie->r = mie->g = mie->b = 0.0;
    double dx = _limit(r, mu) / (double)(INSCATTER_INTEGRAL_SAMPLES);
    Color rayi;
    Color miei;
    _integrand1(r, mu, muS, nu, 0.0, &rayi, &miei);
    int i;
    for (i = 1; i <= INSCATTER_INTEGRAL_SAMPLES; ++i)
    {
        double xj = (double)(i) * dx;
        Color rayj;
        Color miej;
        _integrand1(r, mu, muS, nu, xj, &rayj, &miej);
        ray->r += (rayi.r + rayj.r) / 2.0 * dx;
        ray->g += (rayi.g + rayj.g) / 2.0 * dx;
        ray->b += (rayi.b + rayj.b) / 2.0 * dx;
        mie->r += (miei.r + miej.r) / 2.0 * dx;
        mie->g += (miei.g + miej.g) / 2.0 * dx;
        mie->b += (miei.b + miej.b) / 2.0 * dx;
        rayi = rayj;
        miei = miej;
    }
    ray->r *= betaR.r;
    ray->g *= betaR.g;
    ray->b *= betaR.b;
    mie->r *= betaMSca.x;
    mie->g *= betaMSca.y;
    mie->b *= betaMSca.z;
}

typedef struct
{
    Texture3D* ray;
    Texture3D* mie;
} Inscatter1Params;

static int _inscatter1Worker(ParallelWork* work, int layer, void* data)
{
    Inscatter1Params* params = (Inscatter1Params*)data;
    UNUSED(work);

    double r;
    Color dhdH;
    _getLayerParams(layer, &r, &dhdH);

    int x, y;
    for (x = 0; x < RES_MU_S * RES_NU; x++)
    {
        /*double dx = (double)x / (double)(RES_MU_S * RES_NU);*/
        for (y = 0; y < RES_MU; y++)
        {
            /*double dy = (double)y / (double)(RES_MU);*/

            Color ray = COLOR_BLACK;
            Color mie = COLOR_BLACK;
            double mu, muS, nu;
            _getMuMuSNu((double)x, (double)y, r, dhdH, &mu, &muS, &nu);
            _inscatter1(r, mu, muS, nu, &ray, &mie);
            /* store separately Rayleigh and Mie contributions, WITHOUT the phase function factor
             * (cf "Angular precision") */
            texture3DSetPixel(params->ray, x, y, layer, ray);
            texture3DSetPixel(params->mie, x, y, layer, mie);
        }
    }
    return 1;
}

/*********************** inscatterS.glsl ***********************/

static Color _inscatterS(double r, double mu, double muS, double nu, int first, Texture2D* deltaE, Texture3D* deltaSR, Texture3D* deltaSM)
{
    Color raymie = COLOR_BLACK;

    double dphi = M_PI / (double)(INSCATTER_SPHERICAL_INTEGRAL_SAMPLES);
    double dtheta = M_PI / (double)(INSCATTER_SPHERICAL_INTEGRAL_SAMPLES);

    r = clamp(r, Rg, Rt);
    mu = clamp(mu, -1.0, 1.0);
    muS = clamp(muS, -1.0, 1.0);
    double var = sqrt(1.0 - mu * mu) * sqrt(1.0 - muS * muS);
    nu = clamp(nu, muS * mu - var, muS * mu + var);

    double cthetamin = -sqrt(1.0 - (Rg / r) * (Rg / r));

    Vector3 v = vec3(sqrt(1.0 - mu * mu), 0.0, mu);
    double sx = v.x == 0.0 ? 0.0 : (nu - muS * mu) / v.x;
    Vector3 s = vec3(sx, sqrt(max(0.0, 1.0 - sx * sx - muS * muS)), muS);

    /* integral over 4.PI around x with two nested loops over w directions (theta,phi) -- Eq (7) */
    int itheta;
    for (itheta = 0; itheta < INSCATTER_SPHERICAL_INTEGRAL_SAMPLES; ++itheta)
    {
        double theta = ((double)(itheta) + 0.5) * dtheta;
        double ctheta = cos(theta);

        double greflectance = 0.0;
        double dground = 0.0;
        Color gtransp = {0.0, 0.0, 0.0, 0.0};
        if (ctheta < cthetamin)
        {
            /* if ground visible in direction w
             * compute transparency gtransp between x and ground */
            greflectance = AVERAGE_GROUND_REFLECTANCE / M_PI;
            dground = -r * ctheta - sqrt(r * r * (ctheta * ctheta - 1.0) + Rg * Rg);
            gtransp = _transmittance3(Rg, -(r * ctheta + dground) / Rg, dground);
        }

        int iphi;
        for (iphi = 0; iphi < 2 * INSCATTER_SPHERICAL_INTEGRAL_SAMPLES; ++iphi)
        {
            double phi = ((double)(iphi) + 0.5) * dphi;
            double dw = dtheta * dphi * sin(theta);
            Vector3 w = vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), ctheta);

            double nu1 = v3Dot(s, w);
            double nu2 = v3Dot(v, w);
            double pr2 = _phaseFunctionR(nu2);
            double pm2 = _phaseFunctionM(nu2);

            /* compute irradiance received at ground in direction w (if ground visible) =deltaE */
            Vector3 gnormal;
            gnormal.x = dground * w.x / Rg;
            gnormal.y = dground * w.y / Rg;
            gnormal.z = (r + dground * w.z) / Rg;
            Color girradiance = _irradiance(deltaE, Rg, v3Dot(gnormal, s));

            Color raymie1; /* light arriving at x from direction w */

            /* first term = light reflected from the ground and attenuated before reaching x, =T.alpha/PI.deltaE */
            raymie1.r = greflectance * girradiance.r * gtransp.r;
            raymie1.g = greflectance * girradiance.g * gtransp.g;
            raymie1.b = greflectance * girradiance.b * gtransp.b;

            /* second term = inscattered light, =deltaS */
            if (first)
            {
                /* first iteration is special because Rayleigh and Mie were stored separately,
                 * without the phase functions factors; they must be reintroduced here */
                double pr1 = _phaseFunctionR(nu1);
                double pm1 = _phaseFunctionM(nu1);
                Color ray1 = _texture4D(deltaSR, r, w.z, muS, nu1);
                Color mie1 = _texture4D(deltaSM, r, w.z, muS, nu1);
                raymie1.r += ray1.r * pr1 + mie1.r * pm1;
                raymie1.g += ray1.g * pr1 + mie1.g * pm1;
                raymie1.b += ray1.b * pr1 + mie1.b * pm1;
            }
            else
            {
                Color col = _texture4D(deltaSR, r, w.z, muS, nu1);
                raymie1.r += col.r;
                raymie1.g += col.g;
                raymie1.b += col.b;
            }

            /* light coming from direction w and scattered in direction v
               = light arriving at x from direction w (raymie1) * SUM(scattering coefficient * phaseFunction)
               see Eq (7) */
            raymie.r += raymie1.r * (betaR.r * exp(-(r - Rg) / HR) * pr2 + betaMSca.x * exp(-(r - Rg) / HM) * pm2) * dw;
            raymie.g += raymie1.g * (betaR.g * exp(-(r - Rg) / HR) * pr2 + betaMSca.y * exp(-(r - Rg) / HM) * pm2) * dw;
            raymie.b += raymie1.b * (betaR.b * exp(-(r - Rg) / HR) * pr2 + betaMSca.z * exp(-(r - Rg) / HM) * pm2) * dw;
        }
    }

    /* output raymie = J[T.alpha/PI.deltaE + deltaS] (line 7 in algorithm 4.1) */
    return raymie;
}

typedef struct
{
    Texture3D* result;
    Texture2D* deltaE;
    Texture3D* deltaSR;
    Texture3D* deltaSM;
    int first;
} jParams;

static int _jWorker(ParallelWork* work, int layer, void* data)
{
    jParams* params = (jParams*)data;
    UNUSED(work);

    double r;
    Color dhdH;
    _getLayerParams(layer, &r, &dhdH);

    int x, y;
    for (x = 0; x < RES_MU_S * RES_NU; x++)
    {
        for (y = 0; y < RES_MU; y++)
        {
            Color raymie;
            double mu, muS, nu;
            _getMuMuSNu((double)x, (double)y, r, dhdH, &mu, &muS, &nu);
            raymie = _inscatterS(r, mu, muS, nu, params->first, params->deltaE, params->deltaSR, params->deltaSM);
            texture3DSetPixel(params->result, x, y, layer, raymie);
        }
    }
    return 1;
}

/*********************** irradianceN.glsl ***********************/

void _irradianceNProg(Texture2D* destination, Texture3D* deltaSR, Texture3D* deltaSM, int first)
{
    int x, y;
    double dphi = M_PI / (double)(IRRADIANCE_INTEGRAL_SAMPLES);
    double dtheta = M_PI / (double)(IRRADIANCE_INTEGRAL_SAMPLES);
    for (x = 0; x < SKY_W; x++)
    {
        for (y = 0; y < SKY_H; y++)
        {
            double r, muS;
            int iphi;
            _getIrradianceRMuS((double)x / SKY_W, (double)y / SKY_H, &r, &muS);
            Vector3 s = vec3(max(sqrt(1.0 - muS * muS), 0.0), 0.0, muS);

            Color result = COLOR_BLACK;
            /* integral over 2.PI around x with two nested loops over w directions (theta,phi) -- Eq (15) */
            for (iphi = 0; iphi < 2 * IRRADIANCE_INTEGRAL_SAMPLES; ++iphi)
            {
                double phi = ((double)(iphi) + 0.5) * dphi;
                int itheta;
                for (itheta = 0; itheta < IRRADIANCE_INTEGRAL_SAMPLES / 2; ++itheta)
                {
                    double theta = ((double)(itheta) + 0.5) * dtheta;
                    double dw = dtheta * dphi * sin(theta);
                    Vector3 w = vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
                    double nu = v3Dot(s, w);
                    if (first)
                    {
                        /* first iteration is special because Rayleigh and Mie were stored separately,
                           without the phase functions factors; they must be reintroduced here */
                        double pr1 = _phaseFunctionR(nu);
                        double pm1 = _phaseFunctionM(nu);
                        Color ray1 = _texture4D(deltaSR, r, w.z, muS, nu);
                        Color mie1 = _texture4D(deltaSM, r, w.z, muS, nu);
                        result.r += (ray1.r * pr1 + mie1.r * pm1) * w.z * dw;
                        result.g += (ray1.g * pr1 + mie1.g * pm1) * w.z * dw;
                        result.b += (ray1.b * pr1 + mie1.b * pm1) * w.z * dw;
                    }
                    else
                    {
                        Color col = _texture4D(deltaSR, r, w.z, muS, nu);
                        result.r += col.r * w.z * dw;
                        result.g += col.g * w.z * dw;
                        result.b += col.b * w.z * dw;
                    }
                }
            }

            texture2DSetPixel(destination, x, y, result);
        }
    }
}

/*********************** inscatterN.glsl ***********************/

typedef struct
{
    Texture3D* destination;
    Texture3D* deltaJ;
} InscatterNParams;

static Color _integrand2(Texture3D* deltaJ, double r, double mu, double muS, double nu, double t)
{
    double ri = sqrt(r * r + t * t + 2.0 * r * mu * t);
    double mui = (r * mu + t) / ri;
    double muSi = (nu * t + muS * r) / ri;
    Color c1, c2;
    c1 = _texture4D(deltaJ, ri, mui, muSi, nu);
    c2 = _transmittance3(r, mu, t);
    c1.r *= c2.r;
    c1.g *= c2.g;
    c1.b *= c2.b;
    c1.a = 1.0;
    return c1;
}

static Color _inscatterN(Texture3D* deltaJ, double r, double mu, double muS, double nu)
{
    Color raymie = COLOR_BLACK;
    double dx = _limit(r, mu) / (double)(INSCATTER_INTEGRAL_SAMPLES);
    Color raymiei = _integrand2(deltaJ, r, mu, muS, nu, 0.0);
    int i;
    for (i = 1; i <= INSCATTER_INTEGRAL_SAMPLES; ++i)
    {
        double xj = (double)(i) * dx;
        Color raymiej = _integrand2(deltaJ, r, mu, muS, nu, xj);
        raymie.r += (raymiei.r + raymiej.r) / 2.0 * dx;
        raymie.g += (raymiei.g + raymiej.g) / 2.0 * dx;
        raymie.b += (raymiei.b + raymiej.b) / 2.0 * dx;
        raymiei = raymiej;
    }
    return raymie;
}

static int _inscatterNWorker(ParallelWork* work, int layer, void* data)
{
    UNUSED(work);

    InscatterNParams* params = data;

    double r;
    Color dhdH;
    _getLayerParams(layer, &r, &dhdH);

    int x, y;
    for (x = 0; x < RES_MU_S * RES_NU; x++)
    {
        for (y = 0; y < RES_MU; y++)
        {
            double mu, muS, nu;
            _getMuMuSNu((double)x, (double)y, r, dhdH, &mu, &muS, &nu);
            texture3DSetPixel(params->destination, x, y, layer, _inscatterN(params->deltaJ, r, mu, muS, nu));
        }
    }
    return 1;
}

/*********************** copyInscatterN.glsl ***********************/

typedef struct
{
    Texture3D* source;
    Texture3D* destination;
} CopyInscatterNParams;

static int _copyInscatterNWorker(ParallelWork* work, int layer, void* data)
{
    CopyInscatterNParams* params = (CopyInscatterNParams*)data;
    UNUSED(work);

    double r;
    Color dhdH;
    _getLayerParams(layer, &r, &dhdH);

    int x, y;
    for (x = 0; x < RES_MU_S * RES_NU; x++)
    {
        for (y = 0; y < RES_MU; y++)
        {
            double mu, muS, nu;
            _getMuMuSNu((double)x, (double)y, r, dhdH, &mu, &muS, &nu);
            Color col1 = texture3DGetLinear(params->source, x / (double)(RES_MU_S * RES_NU), y / (double)(RES_MU), layer + 0.5 / (double)(RES_R));
            Color col2 = texture3DGetPixel(params->destination, x, y, layer);
            col2.r += col1.r / _phaseFunctionR(nu);
            col2.g += col1.g / _phaseFunctionR(nu);
            col2.b += col1.b / _phaseFunctionR(nu);
            texture3DSetPixel(params->destination, x, y, layer, col2);
        }
    }
    return 1;
}

/*********************** Final getters ***********************/

static inline Color _applyInscatter(Color inscatter, Color attmod, Color samp)
{
    inscatter.r = inscatter.r - attmod.r * samp.r;
    inscatter.g = inscatter.g - attmod.g * samp.g;
    inscatter.b = inscatter.b - attmod.b * samp.b;
    inscatter.a = inscatter.a - attmod.a * samp.a;
    return vec4max(inscatter, 0.0);
}

/* inscattered light along ray x+tv, when sun in direction s (=S[L]-T(x,x0)S[L]|x0) */
static Color _getInscatterColor(Vector3* _x, double* _t, Vector3 v, Vector3 s, double* _r, double* _mu, Vector3* attenuation)
{
    Color result;
    double r = v3Norm(*_x);
    double mu = v3Dot(*_x, v) / r;
    double d = -r * mu - sqrt(r * r * (mu * mu - 1.0) + Rt * Rt);
    if (d > 0.0)
    {
        /* if x in space and ray intersects atmosphere
           move x to nearest intersection of ray with top atmosphere boundary */
        _x->x += d * v.x;
        _x->y += d * v.y;
        _x->z += d * v.z;
        *_t -= d;
        mu = (r * mu + d) / Rt;
        r = Rt;
    }
    double t = *_t;
    Vector3 x = *_x;
    if (r <= Rt)
    {
        /* if ray intersects atmosphere */
        double nu = v3Dot(v, s);
        double muS = v3Dot(x, s) / r;
        double phaseR = _phaseFunctionR(nu);
        double phaseM = _phaseFunctionM(nu);
        Color inscatter = vec4max(_texture4D(_inscatterTexture, r, mu, muS, nu), 0.0);
        if (t > 0.0)
        {
            Vector3 x0 = v3Add(x, v3Scale(v, t));
            double r0 = v3Norm(x0);
            double rMu0 = v3Dot(x0, v);
            double mu0 = rMu0 / r0;
            double muS0 = v3Dot(x0, s) / r0;
            /* avoids imprecision problems in transmittance computations based on textures */
            *attenuation = _analyticTransmittance(r, mu, t);
            if (r0 > Rg + 0.01)
            {
                /* computes S[L]-T(x,x0)S[L]|x0 */
                Color attmod = {attenuation->x, attenuation->y, attenuation->z, attenuation->x};
                Color samp = _texture4D(_inscatterTexture, r0, mu0, muS0, nu);
                inscatter = _applyInscatter(inscatter, attmod, samp);
                /* avoids imprecision problems near horizon by interpolating between two points above and below horizon */
                const double EPS = 0.004;
                double muHoriz = -sqrt(1.0 - (Rg / r) * (Rg / r));
                if (fabs(mu - muHoriz) < EPS)
                {
                    double a = ((mu - muHoriz) + EPS) / (2.0 * EPS);

                    mu = muHoriz - EPS;
                    r0 = sqrt(r * r + t * t + 2.0 * r * t * mu);
                    mu0 = (r * mu + t) / r0;
                    Color inScatter0 = _texture4D(_inscatterTexture, r, mu, muS, nu);
                    Color inScatter1 = _texture4D(_inscatterTexture, r0, mu0, muS0, nu);
                    Color inScatterA = _applyInscatter(inScatter0, attmod, inScatter1);

                    mu = muHoriz + EPS;
                    r0 = sqrt(r * r + t * t + 2.0 * r * t * mu);
                    mu0 = (r * mu + t) / r0;
                    inScatter0 = _texture4D(_inscatterTexture, r, mu, muS, nu);
                    inScatter1 = _texture4D(_inscatterTexture, r0, mu0, muS0, nu);
                    Color inScatterB = _applyInscatter(inScatter0, attmod, inScatter1);

                    inscatter = vec4mix(inScatterA, inScatterB, a);
                }
            }
        }
        /* avoids imprecision problems in Mie scattering when sun is below horizon */
        inscatter.a *= smoothstep(0.00, 0.02, muS);
        Color mie = _getMie(inscatter);
        result.r = inscatter.r * phaseR + mie.r * phaseM;
        result.g = inscatter.g * phaseR + mie.g * phaseM;
        result.b = inscatter.b * phaseR + mie.b * phaseM;
        result.a = 1.0;
        _fixVec4Min(&result, 0.0);
    }
    else
    {
        /* x in space and ray looking in space */
        result = COLOR_BLACK;
    }

    *_r = r;
    *_mu = mu;
    result.r *= ISun;
    result.g *= ISun;
    result.b *= ISun;
    result.a = 1.0;
    return result;
}

/*ground radiance at end of ray x+tv, when sun in direction s
 *attenuated bewteen ground and viewer (=R[L0]+R[L*]) */
static Color _groundColor(Color base, Vector3 x, double t, Vector3 v, Vector3 s, double r, double mu, Vector3 attenuation)
{
    Color result;

#if 0
    /* ground reflectance at end of ray, x0 */
    Vector3 x0 = v3Add(x, v3Scale(v, t));
    float r0 = v3Norm(x0);
    Vector3 n = v3Scale(x0, 1.0 / r0);

    /* direct sun light (radiance) reaching x0 */
    float muS = v3Dot(n, s);
    Color sunLight = _transmittanceWithShadow(r0, muS);

    /* precomputed sky light (irradiance) (=E[L*]) at x0 */
    Color groundSkyLight = _irradiance(_irradianceTexture, r0, muS);

    /* light reflected at x0 (=(R[L0]+R[L*])/T(x,x0)) */
    Color groundColor;
    groundColor.r = base.r * 0.2 * (max(muS, 0.0) * sunLight.r + groundSkyLight.r) * ISun / M_PI;
    groundColor.g = base.g * 0.2 * (max(muS, 0.0) * sunLight.g + groundSkyLight.g) * ISun / M_PI;
    groundColor.b = base.b * 0.2 * (max(muS, 0.0) * sunLight.b + groundSkyLight.b) * ISun / M_PI;

    /* water specular color due to sunLight */
    /*if (reflectance.w > 0.0)
    {
        vec3 h = normalize(s - v);
        float fresnel = 0.02 + 0.98 * pow(1.0 - dot(-v, h), 5.0);
        float waterBrdf = fresnel * pow(max(dot(h, n), 0.0), 150.0);
        groundColor += reflectance.w * max(waterBrdf, 0.0) * sunLight * ISun;
    }*/
#else
    Color groundColor = base;
#endif

    result.r = attenuation.x * groundColor.r; //=R[L0]+R[L*]
    result.g = attenuation.y * groundColor.g;
    result.b = attenuation.z * groundColor.b;
    result.a = 1.0;

    return result;
}

/* direct sun light for ray x+tv, when sun in direction s (=L0) */
static Color _sunColor(Vector3 v, Vector3 s, double r, double mu)
{
    Color transmittance = r <= Rt ? _transmittanceWithShadow(r, mu) : COLOR_WHITE; /* T(x,xo) */
    double isun = step(cos(M_PI / 180.0), v3Dot(v, s)) * ISun; /* Lsun */
    transmittance.r *= isun;
    transmittance.g *= isun;
    transmittance.b *= isun;
    transmittance.a = 1.0;
    return transmittance; /* Eq (9) */
}

/*********************** Cache/debug methods ***********************/

static int _tryLoadCache2D(Texture2D* tex, const char* tag, int order)
{
    CacheFile* cache;
    int xsize, ysize;

    texture2DGetSize(tex, &xsize, &ysize);
    cache = cacheFileCreateAccessor("atmo-br", "cache", tag, xsize, ysize, 0, order);
    if (cacheFileIsReadable(cache))
    {
        PackStream* stream;
        stream = packReadFile(cacheFileGetPath(cache));
        texture2DLoad(stream, tex);
        packCloseStream(stream);

        cacheFileDeleteAccessor(cache);
        return 1;
    }
    else
    {
        cacheFileDeleteAccessor(cache);
        return 0;
    }
}

static void _saveCache2D(Texture2D* tex, const char* tag, int order)
{
    CacheFile* cache;
    int xsize, ysize;

    texture2DGetSize(tex, &xsize, &ysize);
    cache = cacheFileCreateAccessor("atmo-br", "cache", tag, xsize, ysize, 0, order);
    if (cacheFileIsWritable(cache))
    {
        PackStream* stream;
        stream = packWriteFile(cacheFileGetPath(cache));
        texture2DSave(stream, tex);
        packCloseStream(stream);
    }
    cacheFileDeleteAccessor(cache);
}

static void _saveDebug2D(Texture2D* tex, const char* tag, int order)
{
    CacheFile* cache;
    int xsize, ysize;

    texture2DGetSize(tex, &xsize, &ysize);
    cache = cacheFileCreateAccessor("atmo-br", "png", tag, xsize, ysize, 0, order);
    if (cacheFileIsWritable(cache))
    {
        texture2DSaveToFile(tex, cacheFileGetPath(cache));
    }
    cacheFileDeleteAccessor(cache);
}

static int _tryLoadCache3D(Texture3D* tex, const char* tag, int order)
{
    CacheFile* cache;
    int xsize, ysize, zsize;

    texture3DGetSize(tex, &xsize, &ysize, &zsize);
    cache = cacheFileCreateAccessor("atmo-br", "cache", tag, xsize, ysize, zsize, order);
    if (cacheFileIsReadable(cache))
    {
        PackStream* stream;
        stream = packReadFile(cacheFileGetPath(cache));
        texture3DLoad(stream, tex);
        packCloseStream(stream);

        cacheFileDeleteAccessor(cache);
        return 1;
    }
    else
    {
        cacheFileDeleteAccessor(cache);
        return 0;
    }
}

static void _saveCache3D(Texture3D* tex, const char* tag, int order)
{
    CacheFile* cache;
    int xsize, ysize, zsize;

    texture3DGetSize(tex, &xsize, &ysize, &zsize);
    cache = cacheFileCreateAccessor("atmo-br", "cache", tag, xsize, ysize, zsize, order);
    if (cacheFileIsWritable(cache))
    {
        PackStream* stream;
        stream = packWriteFile(cacheFileGetPath(cache));
        texture3DSave(stream, tex);
        packCloseStream(stream);
    }
    cacheFileDeleteAccessor(cache);
}

static void _saveDebug3D(Texture3D* tex, const char* tag, int order)
{
    CacheFile* cache;
    int xsize, ysize, zsize;

    texture3DGetSize(tex, &xsize, &ysize, &zsize);
    cache = cacheFileCreateAccessor("atmo-br", "png", tag, xsize, ysize, zsize, order);
    if (cacheFileIsWritable(cache))
    {
        texture3DSaveToFile(tex, cacheFileGetPath(cache));
    }
    cacheFileDeleteAccessor(cache);
}

/*********************** Public methods ***********************/
void brunetonInit()
{
    int x, y, z, order;
    ParallelWork* work;

    assert(_inscatterTexture == NULL);

    /* TODO Deletes */
    _transmittanceTexture = texture2DCreate(TRANSMITTANCE_W, TRANSMITTANCE_H);
    _irradianceTexture = texture2DCreate(SKY_W, SKY_H);
    _inscatterTexture = texture3DCreate(RES_MU_S * RES_NU, RES_MU, RES_R);

    /* try loading from cache */
    if (_tryLoadCache2D(_transmittanceTexture, "transmittance", 0)
     && _tryLoadCache2D(_irradianceTexture, "irradiance", 0)
     && _tryLoadCache3D(_inscatterTexture, "inscatter", 0))
    {
        return;
    }

    Texture2D* _deltaETexture = texture2DCreate(SKY_W, SKY_H);
    Texture3D* _deltaSMTexture = texture3DCreate(RES_MU_S * RES_NU, RES_MU, RES_R);
    Texture3D* _deltaSRTexture = texture3DCreate(RES_MU_S * RES_NU, RES_MU, RES_R);
    Texture3D* _deltaJTexture = texture3DCreate(RES_MU_S * RES_NU, RES_MU, RES_R);

    /* computes transmittance texture T (line 1 in algorithm 4.1) */
    _precomputeTransmittanceTexture();
    _saveDebug2D(_transmittanceTexture, "transmittance", 0);

    /* computes irradiance texture deltaE (line 2 in algorithm 4.1) */
    _precomputeIrrDeltaETexture(_deltaETexture);
    _saveDebug2D(_deltaETexture, "deltaE", 0);

    /* computes single scattering texture deltaS (line 3 in algorithm 4.1)
     * Rayleigh and Mie separated in deltaSR + deltaSM */
    Inscatter1Params params = {_deltaSRTexture, _deltaSMTexture};
    work = parallelWorkCreate(_inscatter1Worker, RES_R, &params);
    parallelWorkPerform(work, -1);
    parallelWorkDelete(work);
    _saveDebug3D(_deltaSRTexture, "deltaSR", 0);
    _saveDebug3D(_deltaSMTexture, "deltaSM", 0);

    /* copies deltaE into irradiance texture E (line 4 in algorithm 4.1) */
    /* ??? all black texture (k=0.0) ??? */
    texture2DFill(_irradianceTexture, COLOR_BLACK);

    /* copies deltaS into inscatter texture S (line 5 in algorithm 4.1) */
    for (x = 0; x < RES_MU_S * RES_NU; x++)
    {
        for (y = 0; y < RES_MU; y++)
        {
            for (z = 0; z < RES_R; z++)
            {
                Color result = texture3DGetPixel(_deltaSRTexture, x, y, z);
                Color mie = texture3DGetPixel(_deltaSMTexture, x, y, z);
                result.a = mie.r;
                texture3DSetPixel(_inscatterTexture, x, y, z, result);
            }
        }
    }
    _saveDebug3D(_inscatterTexture, "inscatter", 0);

    /* loop for each scattering order (line 6 in algorithm 4.1) */
    for (order = 2; order <= 4; ++order)
    {
        /* computes deltaJ (line 7 in algorithm 4.1) */
        jParams jparams = {_deltaJTexture, _deltaETexture, _deltaSRTexture, _deltaSMTexture, order == 2};
        work = parallelWorkCreate(_jWorker, RES_R, &jparams);
        parallelWorkPerform(work, -1);
        parallelWorkDelete(work);
        _saveDebug3D(_deltaJTexture, "deltaJ", order);

        /* computes deltaE (line 8 in algorithm 4.1) */
        _irradianceNProg(_deltaETexture, _deltaSRTexture, _deltaSMTexture, order == 2);
        _saveDebug2D(_deltaETexture, "deltaE", order);

        /* computes deltaS (line 9 in algorithm 4.1) */
        InscatterNParams iparams = {_deltaSRTexture, _deltaJTexture};
        work = parallelWorkCreate(_inscatterNWorker, RES_R, &iparams);
        parallelWorkPerform(work, -1);
        parallelWorkDelete(work);
        _saveDebug3D(_deltaSRTexture, "deltaSR", order);

        /* adds deltaE into irradiance texture E (line 10 in algorithm 4.1) */
        texture2DAdd(_deltaETexture, _irradianceTexture);
        _saveDebug2D(_irradianceTexture, "irradiance", order);

        /* adds deltaS into inscatter texture S (line 11 in algorithm 4.1) */
        CopyInscatterNParams cparams = {_deltaSRTexture, _inscatterTexture};
        work = parallelWorkCreate(_copyInscatterNWorker, RES_R, &cparams);
        parallelWorkPerform(work, -1);
        parallelWorkDelete(work);
        _saveDebug3D(_inscatterTexture, "inscatter", order);
    }

    _saveCache2D(_transmittanceTexture, "transmittance", 0);
    _saveCache2D(_irradianceTexture, "irradiance", 0);
    _saveCache3D(_inscatterTexture, "inscatter", 0);

    texture2DDelete(_deltaETexture);
    texture3DDelete(_deltaSMTexture);
    texture3DDelete(_deltaSRTexture);
    texture3DDelete(_deltaJTexture);
}

Color brunetonGetSkyColor(AtmosphereDefinition* definition, Vector3 eye, Vector3 direction, Vector3 sun_position)
{
    UNUSED(definition);

    Vector3 x = {0.0, Rg + (max(eye.y, 0.0) + GROUND_OFFSET) * WORLD_SCALING, 0.0};
    Vector3 v = v3Normalize(direction);
    Vector3 s = v3Normalize(v3Sub(sun_position, x));

    double r = v3Norm(x);
    double mu = v3Dot(x, v) / r;
    double t = -r * mu - sqrt(r * r * (mu * mu - 1.0) + Rg * Rg);

    Vector3 attenuation;
    Color inscatterColor = _getInscatterColor(&x, &t, v, s, &r, &mu, &attenuation); /* S[L]-T(x,xs)S[l]|xs */
    Color sunColor = _sunColor(v, s, r, mu); /* L0 */

    inscatterColor.r += sunColor.r;
    inscatterColor.g += sunColor.g;
    inscatterColor.b += sunColor.b;

    return inscatterColor; /* Eq (16) */
}

Color brunetonApplyAerialPerspective(Renderer* renderer, Vector3 location, Color base)
{
    Vector3 eye = renderer->camera_location;
    Vector3 direction = v3Scale(v3Sub(location, eye), WORLD_SCALING);
    Vector3 sun_position = v3Scale(renderer->atmosphere->getSunDirection(renderer), 149597870.0);

    Vector3 x = {0.0, Rg + (max(eye.y, 0.0) + GROUND_OFFSET) * WORLD_SCALING, 0.0};
    Vector3 v = v3Normalize(direction);
    Vector3 s = v3Normalize(v3Sub(sun_position, x));

    if (v.y == 0.0)
    {
        v.y = -0.000001;
    }

    double r = v3Norm(x);
    double mu = v3Dot(x, v) / r;
    double t = v3Norm(direction);

    Vector3 attenuation;
    Color inscatterColor = _getInscatterColor(&x, &t, v, s, &r, &mu, &attenuation); /* S[L]-T(x,xs)S[l]|xs */
    Color groundColor = _groundColor(base, x, t, v, s, r, mu, attenuation); /*R[L0]+R[L*]*/

    groundColor.r += inscatterColor.r;
    groundColor.g += inscatterColor.g;
    groundColor.b += inscatterColor.b;

    return groundColor; /* Eq (16) */
}

void brunetonGetLightingStatus(Renderer* renderer, LightStatus* status, Vector3 normal, int opaque)
{
    LightDefinition sun, irradiance;
    double muS;

    double r0 = Rg + (max(lightingGetStatusLocation(status).y, 0.0) + GROUND_OFFSET) * WORLD_SCALING;
    Vector3 up = {0.0, 1.0, 0.0};
    Vector3 sun_position = v3Scale(renderer->atmosphere->getSunDirection(renderer), 149597870.0);
    Vector3 x = {0.0, r0, 0.0};
    Vector3 s = v3Normalize(v3Sub(sun_position, x));

    muS = v3Dot(up, s);
    sun.color = _transmittanceWithShadow(r0, muS);
    sun.color.r *= 100.0;
    sun.color.g *= 100.0;
    sun.color.b *= 100.0;
    sun.direction = s;
    sun.reflection = 1.0;
    sun.altered = 1;

    lightingPushLight(status, &sun);

    irradiance.color = _irradiance(_irradianceTexture, r0, muS);
    irradiance.color.r *= 100.0;
    irradiance.color.g *= 100.0;
    irradiance.color.b *= 100.0;
    irradiance.direction = v3Scale(normal, -1.0);
    irradiance.reflection = 0.0;
    irradiance.altered = 0;

    lightingPushLight(status, &irradiance);
}
