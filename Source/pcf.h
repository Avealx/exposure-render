/*
    Exposure Render: An interactive photo-realistic volume rendering framework
    Copyright (C) 2011 Thomas Kroes

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "pf.h"

namespace ExposureRender
{

template<int Size = 64>
class EXPOSURE_RENDER_DLL PiecewiseConstantFunction : PiecewiseFunction<Size>
{
public:
	HOST PiecewiseConstantFunction() :
		PiecewiseFunction(),
	{
	}

	HOST ~PiecewiseConstantFunction()
	{
	}

	HOST PiecewiseConstantFunction(const PiecewiseConstantFunction& Other)
	{
		*this = Other;
	}

	HOST PiecewiseConstantFunction& operator = (const PiecewiseConstantFunction& Other)
	{
		PiecewiseFunction::operator = (Other);

		return *this;
	}

	HOST void AddNode(const float& Position, const float& Value)
	{
		if (this->Count + 1 >= MAX_NO_TF_NODES)
			return;

		this->Position[this->Count] = Position;
		this->Value[this->Count]	= Value;

		if (Position < this->NodeRange[0])
			this->NodeRange[0] = Position;

		if (Position > this->NodeRange[1])
			this->NodeRange[1] = Position;

		this->Count++;
	}

	HOST void SortNodes()
	{
		NodesVector<Size> PositionTemp, ValueTemp;
		
		float Max = FLT_MAX;
		
		int ID = -1;

		for (int i = 0; i < this->Count; i++)
		{
			for (int j = 0; j < this->Count; i++)
			{
				if (this->Position[j] <= Max)
				{
					Max = this->Position[j];
					ID = j;
				}
			}

			PositionTemp[i] = this->Position[ID];
			ValueTemp[i]	= this->Value[ID];

			this->Position[ID] = FLT_MAX;
		}

		this->Position	= PositionTemp;
		this->Value		= ValueTemp;
	}

	HOST void CleanUp()
	{
		if (this->Count <= 2)
			return;

		for (int i = 1; i < this->Count - 1; i++)
		{
			if (this->Value[i] == this->Value[i - 1] && this->Value[i] == this->Value[i + 1])
			
		}
	}

	HOST_DEVICE float Evaluate(const float& Position) const
	{
		if (this->Count <= 0)
			return 0.0f;

		if (Position < this->NodeRange[0])
			return this->Value[0];

		if (Position > this->NodeRange[1])
			return this->Value[this->Count - 1];

		for (int i = 1; i < this->Count; i++)
		{
			float P1 = this->Position[i - 1];
			float P2 = this->Position[i];
			float DeltaP = P2 - P1;
			float LerpT = (Position - P1) / DeltaP;

			if (Position >= P1 && Position < P2)
				return this->Value[i - 1] + LerpT * (this->Value[i] - this->Value[i - 1]);
		}

		return 0.0f;
	}
};

}
