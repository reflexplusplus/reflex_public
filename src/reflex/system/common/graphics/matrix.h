#include "../[require].h"




//
//macros

#define REFLEX_MTX_IDX(x, y) y + (x  * 4)

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

inline const Float32 kPie = 3.14159265f;

inline const Float32 kPie2 = kPie * 2.0f;

REFLEX_INLINE void ResetMatrix(Float32 * matrix)
{
	matrix[REFLEX_MTX_IDX(0,0)] = 1.0f;
	matrix[REFLEX_MTX_IDX(1,0)] = 0.0f;
	matrix[REFLEX_MTX_IDX(2,0)] = 0.0f;
	matrix[REFLEX_MTX_IDX(3,0)] = 0.0f;

	matrix[REFLEX_MTX_IDX(0,1)] = 0.0f;
	matrix[REFLEX_MTX_IDX(1,1)] = 1.0f;
	matrix[REFLEX_MTX_IDX(2,1)] = 0.0f;
	matrix[REFLEX_MTX_IDX(3,1)] = 0.0f;

	matrix[REFLEX_MTX_IDX(0,2)] = 0.0f;
	matrix[REFLEX_MTX_IDX(1,2)] = 0.0f;
	matrix[REFLEX_MTX_IDX(2,2)] = 1.0f;
	matrix[REFLEX_MTX_IDX(3,2)] = 0.0f;

	matrix[REFLEX_MTX_IDX(0,3)] = 0.0f;
	matrix[REFLEX_MTX_IDX(1,3)] = 0.0f;
	matrix[REFLEX_MTX_IDX(2,3)] = 0.0f;
	matrix[REFLEX_MTX_IDX(3,3)] = 1.0f;
}

[[maybe_unused]] void CreateRotateMatrix(Float32 * rotMat, Float32 angle)
{
	Float32 x = 0.0f;
	Float32 y = 0.0f;
	Float32 z = 1.0f;

	Float32 mag = sqrt(x * x + y * y + z * z);

	Float32 sinAngle = sin(angle * kPie / 180.0f);

	Float32 cosAngle = cos(angle * kPie / 180.0f);

	GLfloat xx, yy, zz, xy, yz, zx, xs, ys, zs;

	x /= mag;
	y /= mag;
	z /= mag;

	xx = x * x;
	yy = y * y;
	zz = z * z;
	xy = x * y;
	yz = y * z;
	zx = z * x;
	xs = x * sinAngle;
	ys = y * sinAngle;
	zs = z * sinAngle;
	Float32 oneMinusCos = 1.0f - cosAngle;

////		|	x^2*(1-c)+c		x*y*(1-c)-z*s	x*z*(1-c)+y*s	0	|
////R = 	|	y*x*(1-c)+z*s	y^2*(1-c)+c		y*z*(1-c)-x*s	0	|
////		|	x*z*(1-c)-y*s	y*z*(1-c)+x*s	z^2*(1-c)+c		0	|
//// 		|	0				0				0				1	|

	rotMat[REFLEX_MTX_IDX(0,0)] = (oneMinusCos * xx) + cosAngle;
	rotMat[REFLEX_MTX_IDX(1,0)] = (oneMinusCos * xy) - zs;
	rotMat[REFLEX_MTX_IDX(2,0)] = (oneMinusCos * zx) + ys;
	rotMat[REFLEX_MTX_IDX(3,0)] = 0.0f;

	rotMat[REFLEX_MTX_IDX(0,1)] = (oneMinusCos * xy) + zs;
	rotMat[REFLEX_MTX_IDX(1,1)] = (oneMinusCos * yy) + cosAngle;
	rotMat[REFLEX_MTX_IDX(2,1)] = (oneMinusCos * yz) - xs;
	rotMat[REFLEX_MTX_IDX(3,1)] = 0.0f;

	rotMat[REFLEX_MTX_IDX(0,2)] = (oneMinusCos * zx) - ys;
	rotMat[REFLEX_MTX_IDX(1,2)] = (oneMinusCos * yz) + xs;
	rotMat[REFLEX_MTX_IDX(2,2)] = (oneMinusCos * zz) + cosAngle;
	rotMat[REFLEX_MTX_IDX(3,2)] = 0.0f;

	rotMat[REFLEX_MTX_IDX(0,3)] = 0.0f;
	rotMat[REFLEX_MTX_IDX(1,3)] = 0.0f;
	rotMat[REFLEX_MTX_IDX(2,3)] = 0.0f;
	rotMat[REFLEX_MTX_IDX(3,3)] = 1.0f;
}

[[maybe_unused]] void TranslateMatrix(Float32 * matrix, Float32 x, Float32 y)
{
	matrix[12] += (matrix[0] * x);
	matrix[13] += (matrix[5] * y);
}

[[maybe_unused]] void ScaleMatrix(Float32 * matrix, Float32 xz, Float32 yz)
{
	matrix[0] *= xz;
	matrix[5] *= yz;
}

[[maybe_unused]] void MultiplyMatrix(Float32 * a, const Float32 * b)
{
	Reflex::Float32 r[16];

	for (int i = 0; i < 16; i += 4)
	{
		REFLEX_LOOP(j, 4)
		{
			r[i+j] = b[i]*a[j] + b[i+1]*a[j+4] + b[i+2]*a[j+8] + b[i+3]*a[j+12];
		}
	}

	REFLEX_LOOP(idx, 16)
	{
		a[idx] = r[idx];
	}
}

//void M4x4_SSE(float *A, float *B, float *C)
//{
//	Float32x4 row1 = _mm_load_ps(&B[0]);
//	Float32x4 row2 = _mm_load_ps(&B[4]);
//	Float32x4 row3 = _mm_load_ps(&B[8]);
//	Float32x4 row4 = _mm_load_ps(&B[12]);
//
//	for(int i=0; i<4; i++)
//	{
//		Float32x4 brod1 = _mm_set1_ps(A[4*i + 0]);
//		Float32x4 brod2 = _mm_set1_ps(A[4*i + 1]);
//		Float32x4 brod3 = _mm_set1_ps(A[4*i + 2]);
//		Float32x4 brod4 = _mm_set1_ps(A[4*i + 3]);
//
//		Float32x4 row = _mm_add_ps(
//					_mm_add_ps(
//						_mm_mul_ps(brod1, row1),
//						_mm_mul_ps(brod2, row2)),
//					_mm_add_ps(
//						_mm_mul_ps(brod3, row3),
//						_mm_mul_ps(brod4, row4)));
//		_mm_store_ps(&C[4*i], row);
//	}
//}

REFLEX_END_INTERNAL
