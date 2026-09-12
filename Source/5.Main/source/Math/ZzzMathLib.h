#ifndef __MATHLIB__
#define __MATHLIB__

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	SIDE_FRONT		0
#define	SIDE_ON			2
#define	SIDE_BACK		1
#define	SIDE_CROSS		-2

#define	Q_PI         	3.14159265358979323846f
#define	ON_EPSILON		0.01
#define	EQUAL_EPSILON	0.001
#define ANGLE_TO_RAD	0.017453292519943294f
#define RAD_TO_ANGLE	57.29577951308232089f

#define swaps( a, b)		( ( a) ^= ( b) ^= ( a) ^= ( b))
inline int VectorCompare (const vec3_t v1, const vec3_t v2)
{
	return (fabsf(v1[0]-v2[0]) <= EQUAL_EPSILON) &&
	       (fabsf(v1[1]-v2[1]) <= EQUAL_EPSILON) &&
	       (fabsf(v1[2]-v2[2]) <= EQUAL_EPSILON);
}

inline int QuaternionCompare (const vec4_t v1, const vec4_t v2)
{
	return (fabsf(v1[0]-v2[0]) <= EQUAL_EPSILON) &&
	       (fabsf(v1[1]-v2[1]) <= EQUAL_EPSILON) &&
	       (fabsf(v1[2]-v2[2]) <= EQUAL_EPSILON) &&
	       (fabsf(v1[3]-v2[3]) <= EQUAL_EPSILON);
}

#define Vector(a,b,c,d) {(d)[0]=a;(d)[1]=b;(d)[2]=c;}
#define Vector4(a,b,c,d,target) {(target)[0]=a;(target)[1]=b;(target)[2]=c;(target)[3]=d;}
#define VectorAvg(a) ( ( (a)[0] + (a)[1] + (a)[2] ) / 3 )
#define VectorSubtract(a,b,c) {(c)[0]=(a)[0]-(b)[0];(c)[1]=(a)[1]-(b)[1];(c)[2]=(a)[2]-(b)[2];}
#define VectorSubtractScaled(a,b,c,d) {(c)[0]=(a)[0]-(b)[0]*(d);(c)[1]=(a)[1]-(b)[1]*(d);(c)[2]=(a)[2]-(b)[2]*(d);}
#define VectorAdd(a,b,c) {(c)[0]=(a)[0]+(b)[0];(c)[1]=(a)[1]+(b)[1];(c)[2]=(a)[2]+(b)[2];}
#define VectorAddScaled(a,b,c,d) {(c)[0]=(a)[0]+(b)[0]*(d);(c)[1]=(a)[1]+(b)[1]*(d);(c)[2]=(a)[2]+(b)[2]*(d);}

#define VectorCopy(a,b) {(b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2];}
#define QuaternionCopy(a,b) {(b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2];(b)[3]=(a)[3];}
#define VectorScale(a,b,c) {(c)[0]=(b)*(a)[0];(c)[1]=(b)*(a)[1];(c)[2]=(b)*(a)[2];}
#define DotProduct(x,y) ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define VectorFill(a,b) { (a)[0]=(b); (a)[1]=(b); (a)[2]=(b);}

inline void SETLIMITS(float& VALUE_,const float MAX_,const float MIN_) 
{
	if(VALUE_>MAX_)
	{
		VALUE_=MAX_; 
	}
	else if(VALUE_<MIN_)
	{
		VALUE_=MIN_; 
	}
}


inline void LInterpolationF(float& fout, const float& f01, const float& f02, const float fWeight)
{
	fout = f01 + ((f02 - f01) * fWeight);
}

void VectorInterpolation( vec3_t& v_out, const vec3_t& v_1, const vec3_t& v_2, const float fWeight );
void VectorInterpolation_F( vec3_t& v_out, 
						  const vec3_t& v_1, 
						  const vec3_t& v_2, 
						  const float fArea, 
						  const float fCurrent );
void VectorInterpolation_W( vec3_t& v_out, 
						   const vec3_t& v_1, 
						   const vec3_t& v_2, 
						   const float fWeight );
void VectorDistanceInterpolation_F( vec3_t& v_out, 
								   const vec3_t& v_in,
								   const float fRate );
float VectorDistance3D(const vec3_t& vPosStart, const vec3_t& vPosEnd);
void VectorDistance3D_Dir(const vec3_t& vPosStart, const vec3_t& vPosEnd, vec3_t& vOut);
float VectorDistance3D_DirDist(const vec3_t& vPosStart, const vec3_t& vPosEnd, vec3_t& vOut);


vec_t Q_rint (vec_t in);
inline float VectorLength(vec3_t v) { return sqrtf (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]); }

void VectorMul (vec3_t va, vec3_t vb, vec3_t vc);
void VectorMulF (const vec3_t vIn01, const float fIn01, vec3_t vOut);
void VectorDivF (const vec3_t vIn01, const float fIn01, vec3_t vOut);
void VectorDivFSelf (vec3_t vInOut, const float fIn01);
void VectorDistNormalize (const vec3_t vInFrom, const vec3_t vInTo, vec3_t vOut);
void VectorMA (vec3_t va, float scale, vec3_t vb, vec3_t vc);

void CrossProduct (vec3_t v1, vec3_t v2, vec3_t cross);
vec_t VectorNormalize (vec3_t v);
void VectorInverse (vec3_t v);

void ClearBounds (vec3_t mins, vec3_t maxs);
void AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs);

inline void AngleMatrix (const vec3_t angles, float (*matrix)[4] )
{
	float angle;
	float sr, sp, sy, cr, cp, cy;
	
	angle = angles[2] * (Q_PI*2.0f / 360.0f);
	sy = sinf(angle);
	cy = cosf(angle);
	angle = angles[1] * (Q_PI*2.0f / 360.0f);
	sp = sinf(angle);
	cp = cosf(angle);
	angle = angles[0] * (Q_PI*2.0f / 360.0f);
	sr = sinf(angle);
	cr = cosf(angle);

	// matrix = (Z * Y) * X
	matrix[0][0] = cp*cy;
	matrix[1][0] = cp*sy;
	matrix[2][0] = -sp;
	matrix[0][1] = sr*sp*cy+cr*-sy;
	matrix[1][1] = sr*sp*sy+cr*cy;
	matrix[2][1] = sr*cp;
	matrix[0][2] = (cr*sp*cy+-sr*-sy);
	matrix[1][2] = (cr*sp*sy+-sr*cy);
	matrix[2][2] = cr*cp;
	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
}

void AngleIMatrix (const vec3_t angles, float matrix[3][4] );
inline void R_ConcatTransforms (const float in1[3][4], const float in2[3][4], float out[3][4])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3];
}

inline void VectorIRotate (const vec3_t in1, const float in2[3][4], vec3_t out)
{
	out[0] = in1[0]*in2[0][0] + in1[1]*in2[1][0] + in1[2]*in2[2][0];
	out[1] = in1[0]*in2[0][1] + in1[1]*in2[1][1] + in1[2]*in2[2][1];
	out[2] = in1[0]*in2[0][2] + in1[1]*in2[1][2] + in1[2]*in2[2][2];
}

inline void VectorRotate (const vec3_t in1, const float in2[3][4], vec3_t out)
{
	out[0] = in1[0]*in2[0][0] + in1[1]*in2[0][1] + in1[2]*in2[0][2];
	out[1] = in1[0]*in2[1][0] + in1[1]*in2[1][1] + in1[2]*in2[1][2];
	out[2] = in1[0]*in2[2][0] + in1[1]*in2[2][1] + in1[2]*in2[2][2];
}

inline void VectorTranslate (const vec3_t in1, const float in2[3][4], vec3_t out)
{
	out[0] = in1[0] + in2[0][3];
	out[1] = in1[1] + in2[1][3];
	out[2] = in1[2] + in2[2][3];
}

inline void VectorTransform (const vec3_t in1, const float in2[3][4], vec3_t out)
{
	out[0] = in1[0]*in2[0][0] + in1[1]*in2[0][1] + in1[2]*in2[0][2] + in2[0][3];
	out[1] = in1[0]*in2[1][0] + in1[1]*in2[1][1] + in1[2]*in2[1][2] + in2[1][3];
	out[2] = in1[0]*in2[2][0] + in1[1]*in2[2][1] + in1[2]*in2[2][2] + in2[2][3];
}

void AngleQuaternion( const vec3_t angles, vec4_t quaternion );

inline void QuaternionMatrix( const vec4_t quaternion, float (*matrix)[4] )
{
	matrix[0][0] = 1.0f - 2.0f * quaternion[1] * quaternion[1] - 2.0f * quaternion[2] * quaternion[2];
	matrix[1][0] = 2.0f * quaternion[0] * quaternion[1] + 2.0f * quaternion[3] * quaternion[2];
	matrix[2][0] = 2.0f * quaternion[0] * quaternion[2] - 2.0f * quaternion[3] * quaternion[1];

	matrix[0][1] = 2.0f * quaternion[0] * quaternion[1] - 2.0f * quaternion[3] * quaternion[2];
	matrix[1][1] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[2] * quaternion[2];
	matrix[2][1] = 2.0f * quaternion[1] * quaternion[2] + 2.0f * quaternion[3] * quaternion[0];

	matrix[0][2] = 2.0f * quaternion[0] * quaternion[2] + 2.0f * quaternion[3] * quaternion[1];
	matrix[1][2] = 2.0f * quaternion[1] * quaternion[2] - 2.0f * quaternion[3] * quaternion[0];
	matrix[2][2] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[1] * quaternion[1];
}

inline void QuaternionSlerp( const vec4_t p, vec4_t q, float t, vec4_t qt )
{
	float cosom = p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];

	if (cosom < 0.0f) {
		cosom = -cosom;
		q[0] = -q[0];
		q[1] = -q[1];
		q[2] = -q[2];
		q[3] = -q[3];
	}

	float sclp, sclq;
	if ((1.0f - cosom) > 0.0001f) {
		float omega = acosf( (cosom > 1.0f) ? 1.0f : cosom );
		float sinom = sinf( omega );
		sclp = sinf( (1.0f - t)*omega) / sinom;
		sclq = sinf( t*omega ) / sinom;
	}
	else {
		sclp = 1.0f - t;
		sclq = t;
	}

	qt[0] = sclp * p[0] + sclq * q[0];
	qt[1] = sclp * p[1] + sclq * q[1];
	qt[2] = sclp * p[2] + sclq * q[2];
	qt[3] = sclp * p[3] + sclq * q[3];
}

void FaceNormalize(vec3_t v1,vec3_t v2,vec3_t v3,vec3_t Normal);

float VectorDistance2D(vec3_t va, vec3_t vb);
#ifdef __cplusplus
}
#endif

#endif
