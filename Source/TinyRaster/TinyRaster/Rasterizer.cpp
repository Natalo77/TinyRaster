/*---------------------------------------------------------------------
*
* Copyright © 2016  Minsi Chen
* E-mail: m.chen@derby.ac.uk
*
* The source is written for the Graphics I and II modules. You are free
* to use and extend the functionality. The code provided here is functional
* however the author does not guarantee its performance.
---------------------------------------------------------------------*/
#include <algorithm>
#include <math.h>

#include "Rasterizer.h"

Rasterizer::Rasterizer(void)
{
	mFramebuffer = NULL;
	mScanlineLUT = NULL;
}

void Rasterizer::ClearScanlineLUT()
{
	Scanline *pScanline = mScanlineLUT;

	for (int y = 0; y < mHeight; y++)
	{
		(pScanline + y)->clear();
		(pScanline + y)->shrink_to_fit();
	}
}

unsigned int Rasterizer::ComputeOutCode(const Vector2 & p, const ClipRect& clipRect)
{
	unsigned int CENTRE = 0x0;
	unsigned int LEFT = 0x1;
	unsigned int RIGHT = 0x1 << 1;
	unsigned int BOTTOM = 0x1 << 2;
	unsigned int TOP = 0x1 << 3;
	unsigned int outcode = CENTRE;
	
	if (p[0] < clipRect.left)
		outcode |= LEFT;
	else if (p[0] >= clipRect.right)
		outcode |= RIGHT;

	if (p[1] < clipRect.bottom)
		outcode |= BOTTOM;
	else if (p[1] >= clipRect.top)
		outcode |= TOP;

	return outcode;
}

bool Rasterizer::ClipLine(const Vertex2d & v1, const Vertex2d & v2, const ClipRect& clipRect, Vector2 & outP1, Vector2 & outP2)
{
	//TODO: EXTRA This is not directly prescribed as an assignment exercise. 
	//However, if you want to create an efficient and robust rasteriser, clipping is a usefull addition.
	//The following code is the starting point of the Cohen-Sutherland clipping algorithm.
	//If you complete its implementation, you can test it by calling prior to calling any DrawLine2D .

	const Vector2 p1 = v1.position;
	const Vector2 p2 = v2.position;
	unsigned int outcode1 = ComputeOutCode(p1, clipRect);
	unsigned int outcode2 = ComputeOutCode(p2, clipRect);

	outP1 = p1;
	outP2 = p2;
	
	bool draw = false;

	return true;
}

void Rasterizer::WriteRGBAToFramebuffer(int x, int y, const Colour4 & colour)
{
	if (x >= mWidth || y >= mHeight || x < 0 || y < 0)
	{
		return;
	}

	PixelRGBA *pixel = mFramebuffer->GetBuffer();
	
	pixel[y*mWidth + x] = colour;
}

Rasterizer::Rasterizer(int width, int height)
{
	//Initialise the rasterizer to its initial state
	mFramebuffer = new Framebuffer(width, height);
	mScanlineLUT = new Scanline[height];
	mWidth = width;
	mHeight = height;

	mBGColour.SetVector(0.0, 0.0, 0.0, 1.0);	//default bg colour is black
	mFGColour.SetVector(1.0, 1.0, 1.0, 1.0);    //default fg colour is white

	mGeometryMode = LINE;
	mFillMode = UNFILLED;
	mBlendMode = NO_BLEND;

	SetClipRectangle(0, mWidth, 0, mHeight);
}

Rasterizer::~Rasterizer()
{
	delete mFramebuffer;
	delete[] mScanlineLUT;
}

void Rasterizer::Clear(const Colour4& colour)
{
	PixelRGBA *pixel = mFramebuffer->GetBuffer();

	SetBGColour(colour);

	int size = mWidth*mHeight;
	
	for(int i = 0; i < size; i++)
	{
		//fill all pixels in the framebuffer with background colour
		*(pixel + i) = mBGColour;
	}
}

void Rasterizer::DrawPoint2D(const Vector2& pt, int size)
{
	int x = pt[0];
	int y = pt[1];
	
	WriteRGBAToFramebuffer(x, y, mFGColour);
}

void Rasterizer::DrawLine2D(const Vertex2d & v1, const Vertex2d & v2, int thickness)
{
#pragma region Ex1.1 - Porting drawline bresenham to tinyRaster

	Vector2 pt1 = v1.position;
	Vector2 pt2 = v2.position;

	bool swap_x = pt2[0] < pt1[0];

	int dx = swap_x ? pt1[0] - pt2[0] : pt2[0] - pt1[0];
	int dy = swap_x ? pt1[1] - pt2[1] : pt2[1] - pt1[1];

	int reflect = dy < 0 ? -1 : 1;
	bool swap_xy = dy*reflect > dx;
	int epsilon = 0;

	int sx = swap_xy ? reflect < 0 ? swap_x ? pt1[1] : pt2[1] : swap_x ? pt2[1] : pt1[1] : swap_x ? pt2[0] : pt1[0];
	int y = swap_xy ? reflect < 0 ? swap_x ? pt1[0] : pt2[0] : swap_x ? pt2[0] : pt1[0] : swap_x ? pt2[1] : pt1[1];
	int ex = swap_xy ? reflect < 0 ? swap_x ? pt2[1] : pt1[1] : swap_x ? pt1[1] : pt2[1] : swap_x ? pt1[0] : pt2[0];

	int x = sx;
	y *= reflect;

	Vector2 temp;

	Colour4 colour1 = v1.colour;
	Colour4 colour2 = v2.colour;

	PixelRGBA *pixel = mFramebuffer->GetBuffer();
	while (x <= ex)
	{
		int tempX = swap_xy ? y*reflect : x;
		int tempY = swap_xy ? x : y*reflect;

		temp.SetVector(tempX, tempY);

		Vector2 numerator;
		Vector2 denominator;
		
		numerator = pt1 - temp;
		denominator = pt2 - pt1;

		
		float t = numerator.Norm() / denominator.Norm();
		Colour4 colour = colour2 * t + colour1 * (1 - t);

		if (mBlendMode == ALPHA_BLEND)
		{
			if (!(tempX >= mWidth || tempY >= mHeight || tempX < 0 || tempY < 0))
			{
				Colour4 existingColour = pixel[tempY*mWidth + tempX];
				colour = (colour * colour[3]) + (existingColour * (1.0f - colour[3]));
			}
		}

		SetFGColour(colour);
		DrawPoint2D(temp);

#pragma region Ex1.2 - Accounting for thicknesses above 1

		if (thickness > 1){
			int pixelsPerSide = ((thickness - 1) * 0.5) + 0.5;

			for (int x = 1; x < pixelsPerSide + 1; x++){
				if (swap_xy){
					temp.SetVector(tempX + x, tempY);
					DrawPoint2D(temp);
					temp.SetVector(tempX - x, tempY);
					DrawPoint2D(temp);
				}
				else{
					temp.SetVector(tempX, tempY + x);
					DrawPoint2D(temp);
					temp.SetVector(tempX, tempY - x);
					DrawPoint2D(temp);
				}
			}
		}
#pragma endregion

		epsilon += swap_xy ? dx : dy*reflect;

		if ((epsilon << 1) >= (swap_xy ? dy*reflect : dx))
		{
			y++;

			epsilon -= swap_xy ? dy*reflect : dx;
		}
		x++;
	}
#pragma endregion
}

void Rasterizer::DrawUnfilledPolygon2D(const Vertex2d * vertices, int count)
{
	int i = 0;
	while (i < count - 1)
	{
		DrawLine2D(vertices[i], vertices[i + 1], 1);
		i += 1;
	}
	DrawLine2D(vertices[count - 1], vertices[0], 1);
}

void Rasterizer::ScanlineFillPolygon2D(const Vertex2d * vertices, int count)
{
	DrawUnfilledPolygon2D(vertices, count);

#pragma region Stage 0.5 - Determining which vertices should be drawn.

	bool* xORAdjacentVertexIsHigher = new bool[count];
	for (int x = 1; x < count - 1; x++)
	{
		if ((vertices[x - 1].position[1] > vertices[x].position[1] ^ vertices[x + 1].position[1] > vertices[x].position[1]) && (vertices[x].position[1] != vertices[x + 1].position[1] && vertices[x].position[1] != vertices[x - 1].position[1]))
		{
			xORAdjacentVertexIsHigher[x] = true;
		}
		else
		{
			xORAdjacentVertexIsHigher[x] = false;
		}
	}
	if ((vertices[count - 1].position[1] > vertices[0].position[1] ^ vertices[1].position[1] > vertices[0].position[1]) && (vertices[0].position[1] != vertices[count - 1].position[1] && vertices[0].position[1] != vertices[1].position[1]))
	{
		xORAdjacentVertexIsHigher[0] = true;
	}
	else
	{
		xORAdjacentVertexIsHigher[0] = false;
	}if ((vertices[0].position[1] > vertices[count - 1].position[1] ^ vertices[count - 2].position[1] > vertices[count - 1].position[1]) && (vertices[count - 1].position[1] != vertices[0].position[1] && vertices[count - 2].position[1] != vertices[count - 1].position[1]))
	{
		xORAdjacentVertexIsHigher[count - 1] = true;
	}
	else
	{
		xORAdjacentVertexIsHigher[count - 1] = false;
	}

#pragma endregion

	ClearScanlineLUT();

#pragma region Stage 1 - Calculating and populating LUT

	std::vector<ScanlineLUTItem> list;

	int numItems;
	
	for (int x = 0; x < mFramebuffer->GetHeight(); x++)
	{
		numItems = 0;
		
		for (int y = 0; y < count; y++)
		{
			int vertex1Num;
			int vertex2Num;
			if (y == count - 1)
			{
				vertex1Num = count - 1;
				vertex2Num = 0;
			}
			else
			{
				vertex1Num = y;
				vertex2Num = y + 1;
			}

			if ((x >= vertices[vertex1Num].position[1] && x < vertices[vertex2Num].position[1]) || (x <= vertices[vertex1Num].position[1] && x > vertices[vertex2Num].position[1]))
			{
				bool canBeCounted = false;

				int xCoOrd = vertices[vertex1Num].position[0] + ((x - vertices[vertex1Num].position[1]) * (vertices[vertex1Num].position[0] - vertices[vertex2Num].position[0]) / (vertices[vertex1Num].position[1] - vertices[vertex2Num].position[1]));
				if (xCoOrd < 0)
				{
					xCoOrd = 0;
				}
				if (xCoOrd > 1280)
				{
					xCoOrd = 1280;
				}

				if (x == vertices[vertex1Num].position[1] || x == vertices[vertex2Num].position[1])
				{
					if (x == vertices[vertex1Num].position[1] && xORAdjacentVertexIsHigher[vertex1Num] == true)
					{
						canBeCounted = true;
					}
					else if (x == vertices[vertex2Num].position[1] && xORAdjacentVertexIsHigher[vertex2Num] == true)
					{
						canBeCounted = true;
					}

				}
				else
				{
					canBeCounted = true;
				}

				if (canBeCounted)
				{
					Colour4 colour1 = vertices[vertex1Num].colour;
					Colour4 colour2 = vertices[vertex2Num].colour;

					Vector2 numerator;
					Vector2 denominator;
					Vector2 temp;

					temp[0] = xCoOrd;
					temp[1] = x;

					numerator = vertices[vertex1Num].position - temp;
					denominator = vertices[vertex2Num].position - vertices[vertex1Num].position;

					float t = numerator.Norm() / denominator.Norm();
					Colour4 colour = colour2 * t + colour1 * (1 - t);

					ScanlineLUTItem newItem;
					newItem.colour = colour;
					newItem.pos_x = xCoOrd;

					list.push_back(newItem);
					numItems++;
				}
				
			}
		}

#pragma endregion

#pragma region Stage 1.5 - Sorting LUT

		ScanlineLUTItem* drawList = new ScanlineLUTItem[list.size()];

		if (numItems > 0)
		{
			for (int x = 0; x < numItems ; x++)
			{
				drawList[x] = list.back();
				list.pop_back();
			}

			int i, j;
			ScanlineLUTItem item;
			for (i = 1; i < numItems; i++)
			{
				item = drawList[i];
				j = i - 1;
				while (j >= 0 && drawList[j].pos_x > item.pos_x)
				{
					drawList[j + 1] = drawList[j];
					j = j - 1;
				}
				drawList[j + 1] = item;
			}


			for (int y = 0; y < numItems; y++)
			{
				mScanlineLUT[x].push_back(drawList[y]);
			}
			mScanlineLUT[x].shrink_to_fit();
		}
		delete[] drawList;
	}

#pragma endregion


#pragma region Stage 2 - Drawing from LUT

	for (int x = 0; x < (mFramebuffer->GetHeight()); x++ )
	{
		if (!mScanlineLUT[x].empty())
		{
			for (int z = 0; z < mScanlineLUT[x].size(); z++)
			{
				Vertex2d v1;
				Vertex2d v2;

				v1.position[0] = (mScanlineLUT[x][z].pos_x);
				v1.position[1] = x;
				v1.colour = mScanlineLUT[x][z].colour;


				v2.position[0] = (mScanlineLUT[x][z + 1].pos_x);
				v2.position[1] = x;
				v2.colour = mScanlineLUT[x][z + 1].colour;


				DrawLine2D(v1, v2, 1);

				z++;
 			}
		}
	}

#pragma endregion

	delete xORAdjacentVertexIsHigher;
}

void Rasterizer::ScanlineInterpolatedFillPolygon2D(const Vertex2d * vertices, int count)
{
	ScanlineFillPolygon2D(vertices, count);
}

void Rasterizer::DrawCircle2D(const Circle2D & inCircle, bool filled)
{
	int nSegments = inCircle.radius;
	nSegments /= (2 * 3.14159);
	if (nSegments % 8 != 0)
	{
		nSegments +=  (16 - (nSegments % 8));
	}


	int pointsPerEighth = nSegments / 8;

	double angle = 360.0f / nSegments;
	

	Vertex2d* vertices = new Vertex2d[nSegments];

	Vertex2d vertex;
	vertex.colour = inCircle.colour;

	//work out point 0
	vertices[0].position[0] = inCircle.centre[0];
	vertices[0].position[1] = inCircle.centre[1] + inCircle.radius;
	vertices[0].colour = inCircle.colour;

	//work out octant 2
	double tempAngle = 90 - angle;
	double radiansAngle;
	int x = 1;
	while (tempAngle > 45)
	{
		radiansAngle = tempAngle * (3.14159 / 180);
		vertex.position[0] = inCircle.radius * cos(radiansAngle) + inCircle.centre[0];
		vertex.position[1] = inCircle.radius * sin(radiansAngle) + inCircle.centre[1];
		vertices[x] = vertex;
		x++;
		tempAngle -= angle;
	}

	//work out point(pointsPerEighth)
	double side = sqrt(0.5 * (inCircle.radius * inCircle.radius));
	vertex.position[0] = inCircle.centre[0] + side;
	vertex.position[1] = inCircle.centre[1] + side;
	vertices[pointsPerEighth] = vertex;

	//translate to octant 1.
	double yDiff = 0.0f;
	double xDiff = 0.0f;
	x = pointsPerEighth - 1;
	int y = 1;
	for (x; x >= 0; x--)
	{
		xDiff = vertices[pointsPerEighth].position[0] - vertices[x].position[0];
		yDiff = vertices[pointsPerEighth].position[1] - vertices[x].position[1];
		vertex.position[0] = vertices[pointsPerEighth].position[0] - yDiff;
		vertex.position[1] = vertices[pointsPerEighth].position[1] - xDiff;
		vertices[pointsPerEighth + y] = vertex;
		y++;
	}

	//translate to octant 7 and 8
	int positionBack;
	int positionFwd;
	

	x = 1;
	for (x; x <= 2 * pointsPerEighth; x++)
	{
		positionBack = 2 * pointsPerEighth - x;
		positionFwd = positionBack + x + x;
		yDiff = vertices[positionBack].position[1] - inCircle.centre[1];
		vertex.position[0] = vertices[positionBack].position[0];
		vertex.position[1] = vertices[positionBack].position[1] - (2 * yDiff);
		vertices[positionFwd] = vertex;
	}

	//translate to left semi circle
	x = 1;
	
	{
		for (x; x <= 4 * pointsPerEighth - 1; x++)
		{
			positionBack = 4 * pointsPerEighth - x;
			positionFwd = positionBack + x + x;
			xDiff = vertices[positionBack].position[0] - inCircle.centre[0];
			vertex.position[0] = vertices[positionBack].position[0] - (2 * xDiff);
			vertex.position[1] = vertices[positionBack].position[1];
			vertices[positionFwd] = vertex;
		}
	}

	//draw depending on filled
	if (filled)
	{
		ScanlineFillPolygon2D(vertices, nSegments);
	}
	else
	{
		DrawUnfilledPolygon2D(vertices, nSegments);
	}
}

Framebuffer *Rasterizer::GetFrameBuffer() const
{
	return mFramebuffer;
}
