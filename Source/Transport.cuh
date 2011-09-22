#pragma once

#include "Geometry.h"
#include "Material.h"
#include "Scene.h"

#include "RayMarching.cuh"
#include "Woodcock.cuh"

DEV CColorXyz EstimateDirectLightBrdf(CScene* pScene, CLight& Light, CLightingSample& LS, const Vec3f& Wo, const Vec3f& Pe, const Vec3f& N, CCudaRNG& Rnd)
{
	CColorXyz Ld = SPEC_BLACK, Li = SPEC_BLACK;
	
	const float D = Density(pScene, Pe);

	CBSDF Bsdf(N, Wo, GetDiffuse(pScene, D).ToXYZ(), GetSpecular(pScene, D).ToXYZ(), 5.0f, 0.0001 * GetRoughness(pScene, D).r);
	
	// Light/shadow ray
	CRay Rl; 

	// Light probability
	float LightPdf = 1.0f, BsdfPdf = 1.0f;
	
	// Incident light direction
	Vec3f Wi, P, Pl;

	CColorXyz F = SPEC_BLACK;
	
	// Sample the light source
 	Li = Light.SampleL(Pe, Rl, LightPdf, LS);
	
	Wi = -Rl.m_D; 

	F = Bsdf.F(Wo, Wi); 

	BsdfPdf	= Bsdf.Pdf(Wo, Wi);

	// Sample the light with MIS
	if (!Li.IsBlack() && BsdfPdf > 0.0f && LightPdf > 0.0f && !FreePathRM(Rl, Rnd, P, pScene, 0))
	{
		const float WeightMIS = PowerHeuristic(1.0f, LightPdf, 1.0f, BsdfPdf);
 
		Ld += F * Li * (AbsDot(Wi, N) * WeightMIS / LightPdf);
	}

	// Sample the BRDF with MIS
	F = Bsdf.SampleF(Wo, Wi, BsdfPdf, LS.m_BsdfSample);
	
	if (!F.IsBlack() && BsdfPdf > 0.0f)
	{
		float WeightMIS = 1.0f;

		LightPdf = Light.Pdf(Pe, Wi);

		if (LightPdf == 0.0f)
			return Ld;

		WeightMIS = PowerHeuristic(1.0f, BsdfPdf, 1.0f, LightPdf);

		CLight* pLight = NULL;

		if (NearestLight(pScene, CRay(Pe, Wi, 0.0f), Li, Pl, &LightPdf) && pLight == &Light)
		{
			if (!Li.IsBlack() && !FreePathRM(CRay(Pl, Normalize(Pe - Pl), 0.0f, (Pe - Pl).Length()), Rnd, P, pScene, 0)) 
			{
				Ld += F * Li * AbsDot(Wi, N) * WeightMIS / BsdfPdf;
			}
		}
	}

	return Ld;
}

DEV CColorXyz EstimateDirectLightPhase(CScene* pScene, CLight& Light, CLightingSample& LS, const Vec3f& Wo, const Vec3f& Pe, const Vec3f& N, CCudaRNG& Rnd)
{
	// Accumulated radiance
	CColorXyz Ld = SPEC_BLACK, Li = SPEC_BLACK;

	const float D = Density(pScene, Pe);

	// Light/shadow ray
	CRay Rl; 

	// Light probability
	float LightPdf = 1.0f, BsdfPdf = INV_4_PI_F;
	
	// Incident light direction
	Vec3f Wi, P, Pl;

	CColorXyz F = GetDiffuse(pScene, D).ToXYZ();
	
	// Sample the light source
 	Li = Light.SampleL(Pe, Rl, LightPdf, LS);
	
	Wi = -Rl.m_D; 

	// Sample the light with MIS
	if (!Li.IsBlack() && LightPdf > 0.0f && !FreePathRM(Rl, Rnd, P, pScene, 0))
	{
		const float WeightMIS = PowerHeuristic(1.0f, LightPdf, 1.0f, BsdfPdf);

		Ld += F * Li * WeightMIS / LightPdf;
	}

	// Sample the phase function with MIS
	Wi = UniformSampleSphere(LS.m_BsdfSample.m_Dir);

	if (!F.IsBlack() && BsdfPdf > 0.0f)
	{
		float WeightMIS = 1.0f;

		LightPdf = Light.Pdf(Pe, Wi);

		if (LightPdf == 0.0f)
			return Ld;

		WeightMIS = PowerHeuristic(1.0f, BsdfPdf, 1.0f, LightPdf);

		CLight* pLight = NULL;

		if (NearestLight(pScene, CRay(Pe, Wi, 0.0f), Li, Pl, &LightPdf) && pLight == &Light)
		{
			if (!Li.IsBlack() && !FreePathRM(CRay(Pl, Normalize(Pe - Pl), 0.0f, (Pe - Pl).Length()), Rnd, P, pScene, 0)) 
			{
				Ld += F * Li * WeightMIS / BsdfPdf;
			}
		}
	}

	return Ld;
}

DEV CColorXyz UniformSampleOneLight(CScene* pScene, const Vec3f& Wo, const Vec3f& Pe, const Vec3f& N, CCudaRNG& Rnd, const bool& Brdf)
{
	// Determine no. lights
	const int NumLights = pScene->m_Lighting.m_NoLights;

	// Exit return zero radiance if no light
 	if (NumLights == 0)
 		return SPEC_BLACK;

	CLightingSample LS;

	// Create light sampler
	LS.LargeStep(Rnd);

	// Choose which light to sample
	const int WhichLight = (int)floorf(LS.m_LightNum * (float)NumLights);

	// Get the light
	CLight& Light = pScene->m_Lighting.m_Lights[WhichLight];

	if (Brdf)
		return (float)NumLights * EstimateDirectLightBrdf(pScene, Light, LS, Wo, Pe, N, Rnd);
	else
		return (float)NumLights * EstimateDirectLightPhase(pScene, Light, LS, Wo, Pe, N, Rnd);
}