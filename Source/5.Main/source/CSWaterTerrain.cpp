//////////////////////////////////////////////////////////////////////////
//  CSWaterTerrain.cpp
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BatchRenderer.h"

#include "ZzzOpenglUtil.h"
#include "zzzInfomation.h"
#include "zzzBmd.h"
#include "zzzObject.h"
#include "zzztexture.h"
#include "zzzCharacter.h"
#include "zzzscene.h"
#include "zzzInterface.h"
#include "zzzinventory.h"
#include "zzzLodTerrain.h"
#include "CSWaterTerrain.h"
#include "GMHellas.h"
#include "MapManager.h"
#include "NewUISystem.h"
#include "GPUContext.h"
#include "CBMu/CBMu_RenderConfig.h"

extern  float   WorldTime;
extern  int     MoveSceneFrame;
extern  float   TerrainMappingAlpha[TERRAIN_SIZE * TERRAIN_SIZE];
extern float   g_chrome[MAX_VERTICES][2];
extern int     WaterTextureNumber;

static float LastWaveStart = 0;
constexpr float WavePeriodMs = 2600.0f;

void CSWaterTerrain::Init(void)
{
	Vector(1.f, -1.f, 1.f, m_vLightVector);

	memset(m_iWaveHeight, 0, sizeof(int) * WATER_TERRAIN_SIZE * WATER_TERRAIN_SIZE * 4);

#if CBMu_ENABLE_GPU_WATER_COMPUTE
	int* gpuBuf = GPUContext::Instance().GetWaterHeightBuffer(0);
	if (gpuBuf) {
		memset(gpuBuf, 0, sizeof(int) * WATER_TERRAIN_SIZE * WATER_TERRAIN_SIZE * 4);
	}
#endif
}

void CSWaterTerrain::Update(void)
{
	if (!gMapManager.InHellas(m_iMapIndex))
		return;

#if CBMu_ENABLE_GPU_WATER_COMPUTE
	int* gpuBuf = GPUContext::Instance().GetWaterHeightBuffer(m_iWaterPage);
	if (gpuBuf)
	{
		if (Hero != nullptr && (WorldTime - LastWaveStart >= WavePeriodMs))
		{
			LastWaveStart = WorldTime;
			int WaveX = ((Hero->PositionX) * 2) + (rand() % 30) - 15;
			int WaveY = ((Hero->PositionY) * 2) + 25;

			addSineWave(WaveX, WaveY, 20, 2, 2000);
		}

		m_iWaterPage ^= 1;
		int heroX = (Hero != nullptr) ? (Hero->PositionX * 2) : 0;
		int heroY = (Hero != nullptr) ? (Hero->PositionY * 2) : 0;
		GPUContext::Instance().DispatchWaterWave(m_iWaterPage, heroX, heroY, WorldTime, true);
		return;
	}
#endif

	if (WorldTime - LastWaveStart >= WavePeriodMs)
	{
		LastWaveStart = WorldTime;
		int WaveX = ((Hero->PositionX) * 2) + (rand() % 30) - 15;
		int WaveY = ((Hero->PositionY) * 2) + 25;

		addSineWave(WaveX, WaveY, 20, 2, 2000);
	}

	m_iWaterPage ^= 1;
	calcWave();
	calcBaseWave();
}

void CSWaterTerrain::Render(void)
{
	if (!gMapManager.InHellas(m_iMapIndex))
		return;

	if (Hero == nullptr) return;
	CreateTerrain((Hero->PositionX * 2), (Hero->PositionY * 2));

	int i, j;
	for (i = 0; i < MAX_WATER_GRID * MAX_WATER_GRID; i++)
	{
		float* Normal = m_Normals[i];
		g_chrome[i][0] = (Normal[2] * 0.5f + 0.1f);
		g_chrome[i][1] = (Normal[1] * 0.5f + 0.5f);
	}

	for (j = 0; j + 2 < m_iTriangleListNum; j += 3)
	{
		int o0 = m_iTriangleList[j + 0];
		int o1 = m_iTriangleList[j + 1];
		int o2 = m_iTriangleList[j + 2];

		vec3_t triVerts[4];
		vec3_t triUvs[4];
		vec4_t triColors[4];

		VectorCopy(m_Vertices[o0], triVerts[0]);
		VectorCopy(m_Vertices[o1], triVerts[1]);
		VectorCopy(m_Vertices[o2], triVerts[2]);
		VectorCopy(m_Vertices[o2], triVerts[3]);

		Vector(g_chrome[o0][1], g_chrome[o0][0], 0.f, triUvs[0]);
		Vector(g_chrome[o1][1], g_chrome[o1][0], 0.f, triUvs[1]);
		Vector(g_chrome[o2][1], g_chrome[o2][0], 0.f, triUvs[2]);
		Vector(g_chrome[o2][1], g_chrome[o2][0], 0.f, triUvs[3]);

		for (int k = 0; k < 4; ++k) {
			triColors[k][0] = 0.2f; triColors[k][1] = 0.5f; triColors[k][2] = 0.65f; triColors[k][3] = 1.0f;
		}
		g_BatchRenderer.AddTerrainCustomQuad(TERRAIN_BATCH_ALPHA, BITMAP_MAPTILE, 0, triVerts, triUvs, triColors);
	}

	for (j = 0; j + 2 < m_iTriangleListNum; j += 3)
	{
		int o0 = m_iTriangleList[j + 0];
		int o1 = m_iTriangleList[j + 1];
		int o2 = m_iTriangleList[j + 2];

		vec3_t triVerts[4];
		vec3_t triUvs[4];
		vec4_t triColors[4];

		VectorCopy(m_Vertices[o0], triVerts[0]);
		VectorCopy(m_Vertices[o1], triVerts[1]);
		VectorCopy(m_Vertices[o2], triVerts[2]);
		VectorCopy(m_Vertices[o2], triVerts[3]);

		Vector(g_chrome[o0][1], g_chrome[o0][0], 0.f, triUvs[0]);
		Vector(g_chrome[o1][1], g_chrome[o1][0], 0.f, triUvs[1]);
		Vector(g_chrome[o2][1], g_chrome[o2][0], 0.f, triUvs[2]);
		Vector(g_chrome[o2][1], g_chrome[o2][0], 0.f, triUvs[3]);

		float a0 = 1.f - DotProduct(m_Normals[o0], m_vLightVector);
		float a1 = 1.f - DotProduct(m_Normals[o1], m_vLightVector);
		float a2 = 1.f - DotProduct(m_Normals[o2], m_vLightVector);

		triColors[0][0] = a0; triColors[0][1] = a0 * 2.5f; triColors[0][2] = a0 * 3.f; triColors[0][3] = 1.0f;
		triColors[1][0] = a1; triColors[1][1] = a1 * 2.5f; triColors[1][2] = a1 * 3.f; triColors[1][3] = 1.0f;
		triColors[2][0] = a2; triColors[2][1] = a2 * 2.5f; triColors[2][2] = a2 * 3.f; triColors[2][3] = 1.0f;
		triColors[3][0] = a2; triColors[3][1] = a2 * 2.5f; triColors[3][2] = a2 * 3.f; triColors[3][3] = 1.0f;

		g_BatchRenderer.AddTerrainCustomQuad(TERRAIN_BATCH_BLEND, BITMAP_MAPTILE + 1, 0, triVerts, triUvs, triColors);
	}
}

void CSWaterTerrain::CreateTerrain(int x, int y)
{
	float fHeight, fHeight1;
	int   offset;
	int   grid = MAX_WATER_GRID / 2;

	m_iTriangleListNum = 0;

#if CBMu_ENABLE_GPU_WATER_COMPUTE
	int* curPage = GPUContext::Instance().GetWaterHeightBuffer(m_iWaterPage);
	int* page2 = GPUContext::Instance().GetWaterHeightBuffer(2);
	int* page3 = GPUContext::Instance().GetWaterHeightBuffer(3);
	if (!curPage || !page2 || !page3) {
		curPage = m_iWaveHeight[m_iWaterPage];
		page2 = m_iWaveHeight[2];
		page3 = m_iWaveHeight[3];
	}
#else
	int* curPage = m_iWaveHeight[m_iWaterPage];
	int* page2 = m_iWaveHeight[2];
	int* page3 = m_iWaveHeight[3];
#endif

	for (int offY = 0, i = y - grid + 6; offY < MAX_WATER_GRID; ++i, offY++)
	{
		for (int offX = 0, j = x - grid - 4; offX < MAX_WATER_GRID; ++j, offX++)
		{
			if (i < 0 || j < 0)
			{
				fHeight = 350.f;
			}
			else if (i >= WATER_TERRAIN_SIZE || j >= WATER_TERRAIN_SIZE)
			{
				fHeight = 350.f;
			}
			else
			{
				offset = j + (i * WATER_TERRAIN_SIZE);

				fHeight = curPage[offset] + 350.f;

				fHeight1 = page2[offset] + 350.f;
				fHeight1 += page3[offset] + 350.f;

				fHeight = (fHeight + fHeight1 / 2.f) / 2.f;
			}

			offset = offX + (offY * MAX_WATER_GRID);
			Vector((float)j * WAVE_SCALE, (float)i * WAVE_SCALE, fHeight, m_Vertices[offset]);
			Vector(0.f, 0.f, 0.f, m_Normals[offset]);

			if (offX >= MAX_WATER_GRID - 1 || offY >= MAX_WATER_GRID - 1)
			{
				VectorCopy(m_Normals[offset - 1], m_Normals[offset]);
				continue;
			}

			if (((offX % 2) == 0 && (offY % 2) == 0) || ((offX % 2) == 1 && (offY % 2) == 1))
			{
				m_iTriangleList[m_iTriangleListNum + 0] = offset;
				m_iTriangleList[m_iTriangleListNum + 1] = offset + 1;
				m_iTriangleList[m_iTriangleListNum + 2] = offset + 1 + MAX_WATER_GRID;

				m_iTriangleList[m_iTriangleListNum + 3] = offset;
				m_iTriangleList[m_iTriangleListNum + 4] = offset + 1 + MAX_WATER_GRID;
				m_iTriangleList[m_iTriangleListNum + 5] = offset + MAX_WATER_GRID;
			}
			else
			{
				m_iTriangleList[m_iTriangleListNum + 0] = offset;
				m_iTriangleList[m_iTriangleListNum + 1] = offset + 1;
				m_iTriangleList[m_iTriangleListNum + 2] = offset + MAX_WATER_GRID;

				m_iTriangleList[m_iTriangleListNum + 3] = offset + 1;
				m_iTriangleList[m_iTriangleListNum + 4] = offset + 1 + MAX_WATER_GRID;
				m_iTriangleList[m_iTriangleListNum + 5] = offset + MAX_WATER_GRID;
			}

			m_iTriangleListNum += 6;
		}
	}

	int     v1, v2, v3;
	vec3_t  normalV;
	int     NormalNum[MAX_WATER_GRID * MAX_WATER_GRID] = { 0, };

	for (int j = 0; j < m_iTriangleListNum; j += 3)
	{
		v1 = m_iTriangleList[j + 0];
		v2 = m_iTriangleList[j + 1];
		v3 = m_iTriangleList[j + 2];

		FaceNormalize(m_Vertices[v1], m_Vertices[v2], m_Vertices[v3], normalV);

		VectorAdd(normalV, m_Normals[v1], m_Normals[v1]);
		VectorAdd(normalV, m_Normals[v2], m_Normals[v2]);
		VectorAdd(normalV, m_Normals[v3], m_Normals[v3]);

		NormalNum[v1]++;
		NormalNum[v2]++;
		NormalNum[v3]++;
	}

	for (int i = 0; i < MAX_WATER_GRID * MAX_WATER_GRID; i++)
	{
		m_Normals[i][0] /= (float)(NormalNum[i]);
		m_Normals[i][1] /= (float)(NormalNum[i]);
		m_Normals[i][2] /= (float)(NormalNum[i]);

		VectorNormalize(m_Normals[i]);
	}
}

void CSWaterTerrain::addSineWave(int x, int y, int radiusX, int radiusY, int height)
{
	int speed = height;

#if CBMu_ENABLE_GPU_WATER_COMPUTE
	int* p = GPUContext::Instance().GetWaterHeightBuffer(m_iWaterPage);
	if (!p) p = &m_iWaveHeight[m_iWaterPage][0];
#else
	int* p = &m_iWaveHeight[m_iWaterPage][0];
#endif

	int   cx, cy;
	int   left, top, right, bottom;
	int   square;
	int   radsquare;
	float length = (1024.f / (float)radiusX) * (1024.f / (float)radiusX);

	if (x < 0)
		x = 1 + radiusX + rand() % (WATER_TERRAIN_SIZE - 2 * radiusX - 1);

	if (y < 0)
		y = 1 + radiusY + rand() % (WATER_TERRAIN_SIZE - 2 * radiusY - 1);

	//  radsquare = (radiusX*radiusX) << 8;
	radsquare = (radiusX * radiusY);

	//  height /= 8;
	left = -radiusX;
	right = radiusX;
	top = -radiusY;
	bottom = radiusY;

	// Perform edge clipping...
	if ((x - radiusX) < 1)
		left -= (x - radiusX - 1);

	if ((y - radiusY) < 1)
		top -= (y - radiusY - 1);

	if ((x + radiusX) > WATER_TERRAIN_SIZE - 1)
		right -= (x + radiusX - WATER_TERRAIN_SIZE + 1);

	if ((y + radiusY) > WATER_TERRAIN_SIZE - 1)
		bottom -= (y + radiusY - WATER_TERRAIN_SIZE + 1);

	for (cy = top; cy < bottom; cy++)
	{
		for (cx = left; cx < right; cx++)
		{
			square = cy * cy + cx * cx;
			if (square < radsquare)
			{
				float dist = sqrtf(square * length);
				int sine = (int)((cos(dist) + 0xffff) * speed) >> 19;
				p[WATER_TERRAIN_SIZE * (cy + y) + cx + x] += sine;
			}
		}
	}
}

void CSWaterTerrain::calcBaseWave(void)
{
	int MaxHeight;
	int offset;
	int HeroX = (Hero->PositionX) * 2;
	int HeroY = (Hero->PositionY) * 2;
	int StartX = max(0, HeroX - (VIEW_WATER_GRID / 2));
	int StartY = max(0, HeroY - (VIEW_WATER_GRID / 2));
	int EndX = min(WATER_TERRAIN_SIZE, HeroX + (VIEW_WATER_GRID / 2));
	int EndY = min(WATER_TERRAIN_SIZE, HeroY + (VIEW_WATER_GRID / 2));

	for (int i = StartY; i < EndY; i++)        //  y
	{
		for (int j = StartX; j < EndX; j++)    //  x
		{
			offset = j + (i * WATER_TERRAIN_SIZE);

			float alpha = 0.f;

			MaxHeight = (int)(sin((WorldTime * 0.005f) + (i * 0.1f) + (j * 0.1f)) * 50 * (1 + alpha));
			m_iWaveHeight[2][offset] = (int)(MaxHeight - sin((WorldTime * 0.003f) + (j * 0.1f) + (i * 0.5f)) * 50 * (1 + alpha));
			MaxHeight = (int)(sin((WorldTime * 0.001f) + (i * 0.5f) + (j * 0.5f)) * 25 * (1 + alpha));
			m_iWaveHeight[3][offset] = (int)(MaxHeight - sin((WorldTime * 0.002f) + (j * 1.f) + (i * 0.3f)) * 25 * (1 + alpha));
		}
	}
}

void CSWaterTerrain::calcWave(void)
{
	int newh;

	int* newptr = &m_iWaveHeight[m_iWaterPage][0];
	int* oldptr = &m_iWaveHeight[m_iWaterPage ^ 1][0];

	int x;
	int y = (WATER_TERRAIN_SIZE - 1) * WATER_TERRAIN_SIZE;

	for (int count = WATER_TERRAIN_SIZE + 1; count < y; count += 2)
	{
		for (x = count + WATER_TERRAIN_SIZE - 2; count < x; count++)
		{
			newh = ((oldptr[count + WATER_TERRAIN_SIZE]
				+ oldptr[count - WATER_TERRAIN_SIZE]
				+ oldptr[count + 1]
				+ oldptr[count - 1]
				) >> 1)
				- newptr[count];

			newptr[count] = newh - (newh >> 4);
		}
	}
}

float CSWaterTerrain::GetWaterTerrain(float xf, float yf)
{
	int x = (int)(xf / TERRAIN_SCALE * 2);
	int y = (int)(yf / TERRAIN_SCALE * 2);

	if (x < 0 || y < 0 || x >= WATER_TERRAIN_SIZE || y >= WATER_TERRAIN_SIZE)
		return 350.0f;

#if CBMu_ENABLE_GPU_WATER_COMPUTE
	int* h0 = GPUContext::Instance().GetWaterHeightBuffer(0);
	int* h1 = GPUContext::Instance().GetWaterHeightBuffer(1);
	int* h2 = GPUContext::Instance().GetWaterHeightBuffer(2);
	int* h3 = GPUContext::Instance().GetWaterHeightBuffer(3);
	if (!h0) {
		h0 = m_iWaveHeight[0];
		h1 = m_iWaveHeight[1];
		h2 = m_iWaveHeight[2];
		h3 = m_iWaveHeight[3];
	}
#else
	int* h0 = m_iWaveHeight[0];
	int* h1 = m_iWaveHeight[1];
	int* h2 = m_iWaveHeight[2];
	int* h3 = m_iWaveHeight[3];
#endif

	int idx = x + (y * WATER_TERRAIN_SIZE);
	float fHeight = (m_iWaterPage ? h1[idx] : h0[idx]) + 350.f;
	float fHeight1 = h2[idx] + 350.f + h3[idx] + 350.f;
	fHeight = (fHeight + fHeight1 / 2.f) / 4.f;

	return fHeight;
}

void CSWaterTerrain::RenderWaterAlphaBitmap(int Texture, float xf, float yf, float SizeX, float SizeY, vec3_t Light, float Rotation, float Alpha, float Height)
{
	if (Alpha == 1.f)
		glColor3fv(Light);
	else
		glColor4f(Light[0], Light[1], Light[2], Alpha);

	vec3_t Angle;
	Vector(0.f, 0.f, Rotation, Angle);
	float Matrix[3][4];
	AngleMatrix(Angle, Matrix);

	BindTexture(Texture);
	float mxf = (xf / TERRAIN_SCALE * 2);
	float myf = (yf / TERRAIN_SCALE * 2);
	int   mxi = (int)(mxf);
	int   myi = (int)(myf);

	float Size;
	if (SizeX >= SizeY)
		Size = SizeX;
	else
		Size = SizeY;
	float TexU = (((float)mxi - mxf) + 0.5f * Size);
	float TexV = (((float)myi - myf) + 0.5f * Size);
	float TexScaleU = 1.f / Size;
	float TexScaleV = 1.f / Size;
	Size = (float)((int)Size + 1);
	float Aspect = SizeX / SizeY;

	for (float y = -Size; y <= Size; y += 1.f)
	{
		for (float x = -Size; x <= Size; x += 1.f)
		{
			vec3_t p1[4], p2[4];
			Vector((TexU + x) * TexScaleU, (TexV + y) * TexScaleV, 0.f, p1[0]);
			Vector((TexU + x + 1.f) * TexScaleU, (TexV + y) * TexScaleV, 0.f, p1[1]);
			Vector((TexU + x + 1.f) * TexScaleU, (TexV + y + 1.f) * TexScaleV, 0.f, p1[2]);
			Vector((TexU + x) * TexScaleU, (TexV + y + 1.f) * TexScaleV, 0.f, p1[3]);
			//bool Clip = false;
			for (int i = 0; i < 4; i++)
			{
				p1[i][0] -= 0.5f;
				p1[i][1] -= 0.5f;
				VectorRotate(p1[i], Matrix, p2[i]);
				p2[i][0] *= Aspect;
				p2[i][0] += 0.5f;
				p2[i][1] += 0.5f;
				//if((p2[i][0]>=0.f && p2[i][0]<=1.f) || (p2[i][1]>=0.f && p2[i][1]<=1.f)) Clip = true;
			}
			//if(Clip==true)
			RenderWaterBitmapTile((float)mxi + x, (float)myi + y, 1.f, 1, p2, false, Alpha, Height);
		}
	}
}

void CSWaterTerrain::RenderWaterBitmapTile(float xf, float yf, float lodf, int lodi, vec3_t c[4], bool LightEnable, float Alpha, float Height)
{
	vec3_t TerrainVertex[4];
	int xi = (int)xf;
	int yi = (int)yf;
	if (xi < 0 || yi < 0 || xi >= TERRAIN_SIZE_MASK || yi >= TERRAIN_SIZE_MASK)
		return;

	float TileScale = WAVE_SCALE;
	float sx = xf * WAVE_SCALE;
	float sy = yf * WAVE_SCALE;
	int TerrainIndex1 = xi + (yi * WATER_TERRAIN_SIZE);
	int TerrainIndex2 = xi + lodi + (yi * WATER_TERRAIN_SIZE);
	int TerrainIndex3 = xi + lodi + ((yi + lodi) * WATER_TERRAIN_SIZE);
	int TerrainIndex4 = xi + ((yi + lodi) * WATER_TERRAIN_SIZE);

#if CBMu_ENABLE_GPU_WATER_COMPUTE
	int* waveBuf = GPUContext::Instance().GetWaterHeightBuffer(0);
	if (!waveBuf) waveBuf = m_iWaveHeight[0];
#else
	int* waveBuf = m_iWaveHeight[0];
#endif

	Vector(sx, sy, waveBuf[TerrainIndex1] + 400.f + Height, TerrainVertex[0]);
	Vector(sx + TileScale, sy, waveBuf[TerrainIndex2] + 400.f + Height, TerrainVertex[1]);
	Vector(sx + TileScale, sy + TileScale, waveBuf[TerrainIndex3] + 400.f + Height, TerrainVertex[2]);
	Vector(sx, sy + TileScale, waveBuf[TerrainIndex4] + 400.f + Height, TerrainVertex[3]);

	vec3_t Light[4];
	if (LightEnable)
	{
		VectorCopy(PrimaryTerrainLight[TerrainIndex1], Light[0]);
		VectorCopy(PrimaryTerrainLight[TerrainIndex2], Light[1]);
		VectorCopy(PrimaryTerrainLight[TerrainIndex3], Light[2]);
		VectorCopy(PrimaryTerrainLight[TerrainIndex4], Light[3]);
	}

	vec3_t waveUvs[4];
	vec4_t waveColors[4];
	for (int i = 0; i < 4; i++)
	{
		Vector(c[i][0], c[i][1], 0.f, waveUvs[i]);
		if (LightEnable)
		{
			waveColors[i][0] = Light[i][0];
			waveColors[i][1] = Light[i][1];
			waveColors[i][2] = Light[i][2];
			waveColors[i][3] = Alpha;
		}
		else
		{
			waveColors[i][0] = 1.f; waveColors[i][1] = 1.f; waveColors[i][2] = 1.f; waveColors[i][3] = Alpha;
		}
	}
	g_BatchRenderer.AddTerrainCustomQuad(TERRAIN_BATCH_BLEND, BITMAP_WATER + WaterTextureNumber, 0, TerrainVertex, waveUvs, waveColors);
}
