///////////////////////////////////////////////////////////////////////////////
// 3D Vector
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

vec3_t vec3_origin = {0,0,0};




void VectorInterpolation( vec3_t& v_out, const vec3_t& v_1, const vec3_t& v_2, const float fWeight )
{	
	LInterpolationF(v_out[0], v_1[0], v_2[0], fWeight);
	LInterpolationF(v_out[1], v_1[1], v_2[1], fWeight);
	LInterpolationF(v_out[2], v_1[2], v_2[2], fWeight);
}

void VectorInterpolation_F( vec3_t& v_out, 
						   const vec3_t& v_1, 
						   const vec3_t& v_2, 
						   const float fArea, 
						   const float fCurrent )
{
	float	fWeight = fCurrent / fArea;
	
	VectorInterpolation( v_out, v_1, v_2, fWeight );
}

void VectorInterpolation_W( vec3_t& v_out, 
						   const vec3_t& v_1, 
						   const vec3_t& v_2, 
						   const float fWeight )
{	
	VectorInterpolation( v_out, v_1, v_2, fWeight );
}

void VectorDistanceInterpolation_F(	vec3_t& v_out, 
									const vec3_t& v_Distance, 
									const float fRate)
{
	v_out[0] = v_Distance[0] * fRate;
	v_out[1] = v_Distance[1] * fRate;
	v_out[2] = v_Distance[2] * fRate;
}

float VectorDistance3D(const vec3_t& vPosStart, const vec3_t& vPosEnd)
{
	vec3_t	v3Dist;
	VectorSubtract(vPosEnd, vPosStart, v3Dist);
	return sqrt(v3Dist[0]*v3Dist[0] + v3Dist[1]*v3Dist[1] +v3Dist[2]*v3Dist[2]);
}

void VectorDistance3D_Dir(const vec3_t& vPosStart, const vec3_t& vPosEnd, vec3_t &vDirDist)
{
	VectorSubtract(vPosStart, vPosEnd, vDirDist);
}

float VectorDistance3D_DirDist(const vec3_t& vPosStart, const vec3_t& vPosEnd, vec3_t& vOut)
{
	VectorSubtract(vPosEnd, vPosStart, vOut);
	return sqrt(vOut[0]*vOut[0] + vOut[1]*vOut[1] +vOut[2]*vOut[2]);
}


vec_t Q_rint (vec_t in)
{
	return floor (in + 0.5);
}

void VectorMul (vec3_t va, vec3_t vb, vec3_t vc)
{
	vc[0] = va[0]*vb[0];
	vc[1] = va[1]*vb[1];
	vc[2] = va[2]*vb[2];
}

void VectorMulF (const vec3_t vIn01, const float fIn01, vec3_t vOut)
{
	vOut[0] = vIn01[0] * fIn01;
	vOut[1] = vIn01[1] * fIn01;
	vOut[2] = vIn01[2] * fIn01;
}

void VectorDivF (const vec3_t vIn01, const float fIn01, vec3_t vOut)
{
#ifdef _DEBUG
	if( fIn01==0.0f )
	{
		assert(0);
	}
#endif // _DEBUG
	vOut[0] = vIn01[0] / fIn01;
	vOut[1] = vIn01[1] / fIn01;
	vOut[2] = vIn01[2] / fIn01;
}

void VectorDivFSelf (vec3_t vInOut, const float fIn01)
{
#ifdef _DEBUG
	if( fIn01==0.0f )
	{
		assert(0);
	}
#endif // _DEBUG
	vInOut[0] = vInOut[0] / fIn01;
	vInOut[1] = vInOut[1] / fIn01;
	vInOut[2] = vInOut[2] / fIn01;
}

void VectorDistNormalize (const vec3_t vInFrom, const vec3_t vInTo, vec3_t vOut)
{
	VectorSubtract( vInTo, vInFrom, vOut );
	VectorNormalize(vOut);
}

void VectorMA (vec3_t va, float scale, vec3_t vb, vec3_t vc)
{
	vc[0] = va[0] + scale*vb[0];
	vc[1] = va[1] + scale*vb[1];
	vc[2] = va[2] + scale*vb[2];
}

void CrossProduct (vec3_t v1, vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

vec_t VectorNormalize (vec3_t v)
{
	int		i;
	float	length;

if ( fabs(v[1] - 0.000215956) < 0.0001)
i=1;

	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrtf (length);
	if (length == 0)
		return 0;
		
	for (i=0 ; i< 3 ; i++)
		v[i] /= length;	

	return length;
}

void VectorInverse (vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void ClearBounds (vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;

	for (i=0 ; i<3 ; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

void AngleIMatrix (const vec3_t angles, float matrix[3][4] )
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	
	angle = angles[2] * (Q_PI*2 / 360);
	sy = sinf(angle);
	cy = cosf(angle);
	angle = angles[1] * (Q_PI*2 / 360);
	sp = sinf(angle);
	cp = cosf(angle);
	angle = angles[0] * (Q_PI*2 / 360);
	sr = sinf(angle);
	cr = cosf(angle);

	// matrix = (Z * Y) * X
	matrix[0][0] = cp*cy;
	matrix[0][1] = cp*sy;
	matrix[0][2] = -sp;
	matrix[1][0] = sr*sp*cy+cr*-sy;
	matrix[1][1] = sr*sp*sy+cr*cy;
	matrix[1][2] = sr*cp;
	matrix[2][0] = (cr*sp*cy+-sr*-sy);
	matrix[2][1] = (cr*sp*sy+-sr*cy);
	matrix[2][2] = cr*cp;
	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
}

void AngleQuaternion( const vec3_t angles, vec4_t quaternion )
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	// FIXME: rescale the inputs to 1/2 angle
	angle = angles[2] * 0.5f;
	sy = sinf(angle);
	cy = cosf(angle);
	angle = angles[1] * 0.5f;
	sp = sinf(angle);
	cp = cosf(angle);
	angle = angles[0] * 0.5f;
	sr = sinf(angle);
	cr = cosf(angle);

	quaternion[0] = sr*cp*cy-cr*sp*sy; // X
	quaternion[1] = cr*sp*cy+sr*cp*sy; // Y
	quaternion[2] = cr*cp*sy-sr*sp*cy; // Z
	quaternion[3] = cr*cp*cy+sr*sp*sy; // W
}

void FaceNormalize(vec3_t v1,vec3_t v2,vec3_t v3,vec3_t Normal)
{
    float nx,ny,nz;
    nx = (v2[1]-v1[1]) * (v3[2]-v1[2]) - (v3[1]-v1[1]) * (v2[2]-v1[2]);
    ny = (v2[2]-v1[2]) * (v3[0]-v1[0]) - (v3[2]-v1[2]) * (v2[0]-v1[0]);
    nz = (v2[0]-v1[0]) * (v3[1]-v1[1]) - (v3[0]-v1[0]) * (v2[1]-v1[1]);
	//if(nx==0.f || ny==0.f || nz==0.f) return;
    double dot = sqrt(nx*nx+ny*ny+nz*nz);
	if(dot==0) return;
	Normal[0] = (nx/dot);
	Normal[1] = (ny/dot);
	Normal[2] = (nz/dot);
}

float VectorDistance2D(vec3_t va, vec3_t vb)
{
	float dx = va[0]-vb[0];
	float dy = va[1]-vb[1];
	return sqrtf(dx*dx+dy*dy);
}
