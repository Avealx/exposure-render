/*
	Copyright (c) 2011, T. Kroes <t.kroes@tudelft.nl>
	All rights reserved.

	Redistribution and use in source and binary forms, with or witDEVut modification, are permitted provided that the following conditions are met:

	- Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	- Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
	- Neither the name of the TU Delft nor the names of its contributors may be used to endorse or promote products derived from this software witDEVut specific prior written permission.
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT DEVLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT DEVLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) DEVWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "Vector.h"

#if defined (__CUDA_ARCH__)
	#include "CudaUtilities.h"
#endif

namespace ExposureRender
{

struct Volume
{
	Volume()
	{
	}

	~Volume()
	{
	}

	HOST Volume& Volume::operator = (const Volume& Other)
	{
		this->Resolution		= Other.Resolution;
		this->Spacing			= Other.Spacing;
		this->NormalizeSize		= Other.NormalizeSize;

		float Scale = 1.0f;

		if (this->NormalizeSize)
		{
			const Vec3f PhysicalSize = Vec3f(this->Resolution[0], this->Resolution[1], this->Resolution[2]) * this->Spacing;
			
			const float Max = max(PhysicalSize[0], max(PhysicalSize[1], PhysicalSize[2]));
			Scale = 1.0f / Max;
		}

		this->InvResolution[0]	= 1.0f / this->Resolution[0];
		this->InvResolution[1]	= 1.0f / this->Resolution[1];
		this->InvResolution[2]	= 1.0f / this->Resolution[2];
		this->Spacing			= Scale * Spacing;
		this->InvSpacing[0]		= 1.0f / Spacing[0];
		this->InvSpacing[1]		= 1.0f / Spacing[1];
		this->InvSpacing[2]		= 1.0f / Spacing[2];
		this->Size				= Vec3f(this->Resolution[0], this->Resolution[1], this->Resolution[2]) * this->Spacing;
		this->InvSize[0]		= 1.0f / this->Size[0];
		this->InvSize[1]		= 1.0f / this->Size[1];
		this->InvSize[2]		= 1.0f / this->Size[2];
		this->MinAABB			= -0.5f * this->Size;
		this->MaxAABB			= 0.5f * this->Size;

		const float MinVoxelSize = min(this->Spacing[0], min(this->Spacing[1], this->Spacing[2]));

		this->GradientDeltaX[0]	= MinVoxelSize;
		this->GradientDeltaX[1]	= 0.0f;
		this->GradientDeltaX[2]	= 0.0f;
		this->GradientDeltaY[0]	= 0.0f;
		this->GradientDeltaY[1]	= MinVoxelSize;
		this->GradientDeltaY[2]	= 0.0f;
		this->GradientDeltaZ[0]	= 0.0f;
		this->GradientDeltaZ[1]	= 0.0f;
		this->GradientDeltaZ[2]	= MinVoxelSize;

#ifdef __CUDA_ARCH__
		// this->Free();
		
		const int NoVoxels = (int)this->Resolution[0] * (int)this->Resolution[1] * (int)this->Resolution[2];

		if (NoVoxels <= 0)
			return *this;

		CUDA::Allocate(this->pVoxels, NoVoxels);
		CUDA::MemCopyHostToDevice(Other.pVoxels, this->pVoxels, NoVoxels);
#endif

		return *this;
	}

	HOST void Free()
	{
#ifdef __CUDA_ARCH__
		if (this->pVoxels != NULL)
			CUDA::Free(this->pVoxels);
#endif
	}

	HOST_DEVICE unsigned short Get(const Vec3i& XYZ) const
	{
		if (!this->pVoxels)
			return unsigned short();
		
		Vec3i ClampedXYZ = XYZ;
		ClampedXYZ.Clamp(Vec3i(0, 0, 0), Vec3i(this->Resolution[0] - 1, this->Resolution[1] - 1, this->Resolution[2] - 1));
		
		return this->pVoxels[ClampedXYZ[2] * (int)this->Resolution[0] * (int)this->Resolution[1] + ClampedXYZ[1] * (int)this->Resolution[0] + ClampedXYZ[0]];
	}

	HOST_DEVICE unsigned short Get(const Vec3f& XYZ) const
	{
		Vec3f LocalXYZ = Vec3f(this->Resolution[0], this->Resolution[1], this->Resolution[2]) * ((XYZ - this->MinAABB) * this->InvSize);

		return this->Get(Vec3i(LocalXYZ[0], LocalXYZ[1], LocalXYZ[2]));
	}
	
	Vec3i				Resolution;			// FIXME
	Vec3f				InvResolution;
	Vec3f				MinAABB;
	Vec3f				MaxAABB;
	Vec3f				Size;
	Vec3f				InvSize;
	bool				NormalizeSize;
	Vec3f				Spacing;
	Vec3f				InvSpacing;
	Vec3f				GradientDeltaX;
	Vec3f				GradientDeltaY;
	Vec3f				GradientDeltaZ;
	float				GradientMagnitudeRange[2];
	unsigned short*		pVoxels;
};

}