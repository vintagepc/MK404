//
//    Copyright 2011 Nicholas Pleschko <nicholas@nigi.li>
//
//    ============================================
//
//    This file is part of SimpleArcballCamera.
//
//    SimpleArcballCamera is free software: you can redistribute it and/or 
//    modify it under the terms of the GNU General Public License as published
//    by the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//    
//    SimpleArcballCamera is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with SimpleArcballCamera.  If not, 
//    see <http://www.gnu.org/licenses/>.
//
//    This file is based on the Arcball implementation of Mario Konrad
//                    <http://www.mario-konrad.ch/index.php?page=20408>

#ifndef SimpleArcballCamera_Matrix_hpp
#define SimpleArcballCamera_Matrix_hpp

#include "Vectors.hpp"
#include <cstring>

namespace math {

class mat2
{
	public:
		mat2(void)
			{ load_identity(); }
		mat2(const mat2 & m)
			{ for (int i=0; i<4; ++i) x[i]=m.x[i]; }
		mat2(float x11, float x12,
				float x21, float x22)
			{ x[0]=x11; x[1]=x12; x[2]=x21; x[3]=x22; }
		mat2(float m[4])
			{ for (int i=0; i<4; ++i) x[i]=m[i]; }
		inline void load_identity(void)
			{ for (int i=0; i<4; ++i) x[i]=(i%3)?0.0:1.0; }
		inline float det(void) const
			{ return	(x[0]*x[3] - x[2]*x[1]); }
		inline mat2 & operator *= (float f)
			{ for (int i=0; i<4; ++i) x[i]*=f; return *this; }
		inline mat2 & operator += (const mat2 & m)
			{ for (int i=0; i<4; ++i) x[i]+=m.x[i]; return *this; }
		inline mat2 & operator -= (const mat2 & m)
			{ for (int i=0; i<4; ++i) x[i]-=m.x[i]; return *this; }
		inline mat2 & operator *= (const mat2 & m)
			{
				float c[4] =
				{
					x[0]*m.x[0] + x[1]*m.x[2], x[0]*m.x[1] + x[1]*m.x[3],
					x[2]*m.x[0] + x[3]*m.x[2], x[2]*m.x[1] + x[3]*m.x[3]
				};
				for (int i=0; i<4; ++i) x[i]=c[i];
				return *this;
			}
		inline float spur(void) const
			{ return x[0]+x[3]; }
		inline mat2 inv(void) const
			{
				float d = det();
				if (d < 1.0e-7 && d > -1.0e-7) return mat2();
				return (1.0 / d) * mat2(x[3], -x[1], -x[2], x[0]);
			}
		inline mat2 transpose(void) const
			{ return mat2(x[0], x[2], x[1], x[3]); }
		inline vec2 rowvec(int row) const	// row in [0..1]
			{ row *= 2; return vec2(x[row], x[row+1]); }
		inline vec2 colvec(int col) const	// col in [0..1]
			{ return vec2(x[col], x[col+2]); }
		friend mat2 operator * (float f, const mat2 & m)
			{ return mat2(m) *= f; }
		friend mat2 operator + (const mat2 & a, const mat2 & b)
			{ return mat2(a) += b; }
		friend mat2 operator - (const mat2 & a, const mat2 & b)
			{ return mat2(a) -= b; }
		friend mat2 operator * (const mat2 & a, const mat2 & b)
			{ return mat2(a) *= b; }
		friend mat2 operator * (const vec2 & v, const mat2 & m)
			{
				float c1 = m.x[0]+m.x[2];
				float c2 = m.x[1]+m.x[3];
				return mat2(
					v[0]*c1, v[0]*c2,
					v[1]*c1, v[1]*c2);
			}
		friend vec2 operator * (const mat2 & m, const vec2 & v)
			{
				return vec2(
					m.x[0]*v[0] + m.x[1]*v[1],
					m.x[2]*v[0] + m.x[3]*v[1]);
			}
	private:
		float x[4];
};

class mat3
{
	public:
		mat3(void)
			{ load_identity(); }
		mat3(const mat3 & m)
			{ for (int i=0; i<9; ++i) x[i]=m.x[i]; }
		mat3(float x11, float x12, float x13,
				float x21, float x22, float x23,
				float x31, float x32, float x33)
			{
				x[0]=x11; x[1]=x12; x[2]=x13;
				x[3]=x21; x[4]=x22; x[5]=x23;
				x[6]=x31; x[7]=x32; x[8]=x33;
			}
		mat3(float m[9])
			{ for (int i=0; i<9; ++i) x[i]=m[i]; }
		inline void load_identity(void)
			{ for (int i=0; i<9; ++i) x[i]=(i%4)?0.0:1.0; }
		inline float det(void) const
			{
				return	x[0] * (x[4]*x[8] - x[7]*x[5])
					-	x[1] * (x[3]*x[8] - x[6]*x[5])
					+	x[2] * (x[3]*x[7] - x[6]*x[4]);
			}
		inline mat3 & operator *= (float f)
			{ for (int i=0; i<9; ++i) x[i]*=f; return *this; }
		inline mat3 & operator += (const mat3 & m)
			{ for (int i=0; i<9; ++i) x[i]+=m.x[i]; return *this; }
		inline mat3 & operator -= (const mat3 & m)
			{ for (int i=0; i<9; ++i) x[i]-=m.x[i]; return *this; }
		inline mat3 & operator *= (const mat3 & m)
			{
				float c[9] =
				{
					x[0]*m.x[0] + x[1]*m.x[3] + x[2]*m.x[6],
					x[0]*m.x[1] + x[1]*m.x[4] + x[2]*m.x[7],
					x[0]*m.x[2] + x[1]*m.x[5] + x[2]*m.x[8],
					x[3]*m.x[0] + x[4]*m.x[3] + x[5]*m.x[6],
					x[3]*m.x[1] + x[4]*m.x[4] + x[5]*m.x[7],
					x[3]*m.x[2] + x[4]*m.x[5] + x[5]*m.x[8],
					x[6]*m.x[0] + x[7]*m.x[3] + x[8]*m.x[6],
					x[6]*m.x[1] + x[7]*m.x[4] + x[8]*m.x[7],
					x[6]*m.x[2] + x[7]*m.x[5] + x[8]*m.x[8]
				};
				for (int i=0; i<9; ++i) x[i]=((c[i]<1.0e-10)&&(c[i]>-1.0e-10))?0.0:c[i];
				return *this;
			}
		inline float spur(void) const
			{ return x[0]+x[4]+x[8]; }
		inline mat3 transpose(void) const
			{
				return mat3(
					x[0], x[3], x[6],
					x[1], x[4], x[7],
					x[2], x[5], x[8]);
			}
		inline vec3 rowvec(int row) const	// row in [0..2]
			{ row *= 3; return vec3(x[row], x[row+1], x[row+2]); }
		inline vec3 colvec(int col) const	// col in [0..2]
			{ return vec3(x[col], x[col+3], x[col+6]); }
		inline mat3 inv(void) const
			{
				float d = det();
				if ((d < 1.0e-7) && (d > -1.0e-7)) return mat3();
				mat3 m(*this), I;
				int i0, i1;
				float q;
				for (int j = 0; j < 3; ++j)
				{
					// Diagonalfeld normalisieren
					q = m.x[j*3+j];
					if (q == 0.0) {
						// Es darf keine 0 mehr in der Diagonalen stehen
						for (int i = j+1; i < 3; ++i) {
							// Suche Reihe mit einem Feld != 0.0 und addiere
							if (m.x[i*3+j] != 0.0) {
								for (int k = 0; k < 3; ++k) {
									i0 = j*3+k; i1 = i*3+k;
									m.x[i0] += m.x[i1];
									I.x[i0] += I.x[i1];
								}
								q = m.x[j*3+j];
								break;
							}
						}
					} else {
						// Diagonalen auf 1 bringen
						for (int k = 0; k < 3; ++k) {
							i0 = j*3+k;
							m.x[i0] /= q;
							I.x[i0] /= q;
						}
					}
					// Spalten ausserhalb der Diagonalen auf 0 bringen
					for (int i = 0; i < 3; ++i) {
						if (i != j) {
							q = m.x[i*3+j];
							for (int k = 0; k < 3; ++k) {
								i0 = i*3+k; i1 = j*3+k;
								m.x[i0] -= q * m.x[i1];
								I.x[i0] -= q * I.x[i1];
							}
						}
					}
				}
				// Test auf Einheitsmatrix, falls nicht => Berechnung nicht gelungen
				for (int i = 0; i < 3; ++i)
					for (int j = 0; j < 3; ++j)
						if (m.x[i*3+j] != ((i==j) ? 1:0)) return mat3();
				return I;
			}
		friend mat3 operator * (float f, const mat3 & m)
			{ return mat3(m) *= f; }
		friend mat3 operator + (const mat3 & a, const mat3 & b)
			{ return mat3(a) += b; }
		friend mat3 operator - (const mat3 & a, const mat3 & b)
			{ return mat3(a) -= b; }
		friend mat3 operator * (const mat3 & a, const mat3 & b)
			{ return mat3(a) *= b; }
		friend mat3 operator * (const vec3 & v, const mat3 & m)
			{
				float c1 = m.x[0]+m.x[3]+m.x[6];
				float c2 = m.x[1]+m.x[4]+m.x[7];
				float c3 = m.x[2]+m.x[5]+m.x[8];
				return mat3(
					v[0]*c1, v[0]*c2, v[0]*c3,
					v[1]*c1, v[1]*c2, v[1]*c3,
					v[2]*c1, v[2]*c2, v[2]*c3);
			}
		friend vec3 operator * (const mat3 & m, const vec3 & v)
			{
				return vec3(
					m.x[0]*v[0] + m.x[1]*v[1] + m.x[2]*v[2],
					m.x[3]*v[0] + m.x[4]*v[1] + m.x[5]*v[2],
					m.x[6]*v[0] + m.x[7]*v[1] + m.x[8]*v[2]);
			}
	private:
		float x[9];
};

class mat4
{
	public:
		mat4(void)
			{ load_identity(); }
		mat4(const mat4 & m)
			{ *this = m; }
		mat4(float x11, float x12, float x13, float x14,
				float x21, float x22, float x23, float x24,
				float x31, float x32, float x33, float x34,
				float x41, float x42, float x43, float x44)
			{
				x[ 0]=x11; x[ 1]=x12; x[ 2]=x13; x[ 3]=x14;
				x[ 4]=x21; x[ 5]=x22; x[ 6]=x23; x[ 7]=x24;
				x[ 8]=x31; x[ 9]=x32; x[10]=x33; x[11]=x34;
				x[12]=x41; x[13]=x42; x[14]=x43; x[15]=x44;
			}
		mat4(const float m[16])
			{ for (int i=0; i<16; ++i) x[i]=m[i]; }
    
        mat4(vec3 modelcenter, vec3 cameraeye, vec3 cameraup)
    {
        // creates the lookat ROTATION WITHOUT TRANSLATION
        
        
        vec3 F = (modelcenter - cameraeye);
        vec3 f = F.normalize();
        
        vec3 s = f.cross(cameraup);
        s = s.normalize();
        
        vec3 u = s.cross(f);
        
        
        x[0]= s[0]; 
        x[1]= s[1]; 
        x[2]= s[2]; 
        
        x[3]= 0;
        
        x[4]= u[0];
        x[5]= u[1];
        x[6]= u[2];
        
        x[7]= 0;
        x[8]= -f[0];
        x[9]= -f[1];
        x[10]= -f[2];
        
        x[11]= 0;
        x[12]= 0; 
        x[13]= 0; 
        x[14]= 0; 
        x[15]= 1;
      
        
       
        
        // First translate the eye position into the center then rotate
        *this =  *this;
    
        
    }
    
		inline void load_identity(void)
			{ for (int i=0; i<16; ++i) x[i]=(i%5)?0.0:1.0; }
		inline float det(void) const
			{	
				return	// Regel von Sarrus
						x[ 0]*x[ 5]*x[10]*x[15]
					-	x[12]*x[ 9]*x[ 6]*x[ 3]
					+	x[ 1]*x[ 6]*x[11]*x[12]
					-	x[13]*x[10]*x[ 7]*x[ 0]
					+	x[ 2]*x[ 7]*x[ 8]*x[13]
					-	x[14]*x[11]*x[ 4]*x[ 1]
					+	x[ 3]*x[ 4]*x[ 9]*x[14]
					-	x[15]*x[ 8]*x[ 5]*x[ 2];
			}
		inline mat4 & operator = (const mat4 & m)
			{ for (int i=0; i<16; ++i) x[i]=m.x[i]; return *this; }
		inline mat4 & operator *= (float f)
			{ for (int i=0; i<16; ++i) x[i]*=f; return *this; }
		inline mat4 & operator += (const mat4 & m)
			{ for (int i=0; i<16; ++i) x[i]+=m.x[i]; return *this; }
		inline mat4 & operator -= (const mat4 & m)
			{ for (int i=0; i<16; ++i) x[i]-=m.x[i]; return *this; }
		inline mat4 & operator *= (const mat4 & m)
			{
				float c[16] =
				{
					x[ 0]*m.x[ 0] + x[ 1]*m.x[ 4] + x[ 2]*m.x[ 8] + x[ 3]*m.x[12],
					x[ 0]*m.x[ 1] + x[ 1]*m.x[ 5] + x[ 2]*m.x[ 9] + x[ 3]*m.x[13],
					x[ 0]*m.x[ 2] + x[ 1]*m.x[ 6] + x[ 2]*m.x[10] + x[ 3]*m.x[14],
					x[ 0]*m.x[ 3] + x[ 1]*m.x[ 7] + x[ 2]*m.x[11] + x[ 3]*m.x[15],
					x[ 4]*m.x[ 0] + x[ 5]*m.x[ 4] + x[ 6]*m.x[ 8] + x[ 7]*m.x[12],
					x[ 4]*m.x[ 1] + x[ 5]*m.x[ 5] + x[ 6]*m.x[ 9] + x[ 7]*m.x[13],
					x[ 4]*m.x[ 2] + x[ 5]*m.x[ 6] + x[ 6]*m.x[10] + x[ 7]*m.x[14],
					x[ 4]*m.x[ 3] + x[ 5]*m.x[ 7] + x[ 6]*m.x[11] + x[ 7]*m.x[15],
					x[ 8]*m.x[ 0] + x[ 9]*m.x[ 4] + x[10]*m.x[ 8] + x[11]*m.x[12],
					x[ 8]*m.x[ 1] + x[ 9]*m.x[ 5] + x[10]*m.x[ 9] + x[11]*m.x[13],
					x[ 8]*m.x[ 2] + x[ 9]*m.x[ 6] + x[10]*m.x[10] + x[11]*m.x[14],
					x[ 8]*m.x[ 3] + x[ 9]*m.x[ 7] + x[10]*m.x[11] + x[11]*m.x[15],
					x[12]*m.x[ 0] + x[13]*m.x[ 4] + x[14]*m.x[ 8] + x[15]*m.x[12],
					x[12]*m.x[ 1] + x[13]*m.x[ 5] + x[14]*m.x[ 9] + x[15]*m.x[13],
					x[12]*m.x[ 2] + x[13]*m.x[ 6] + x[14]*m.x[10] + x[15]*m.x[14],
					x[12]*m.x[ 3] + x[13]*m.x[ 7] + x[14]*m.x[11] + x[15]*m.x[15]
				};
				for (int i=0; i<16; ++i) x[i]=((c[i]<1.0e-10)&&(c[i]>-1.0e-10))?0.0:c[i];
				return *this;
			}
		inline float & operator[](size_t i)
			{ return x[i]; }
		inline float operator[](size_t i) const
			{ return x[i]; }
		inline const float * get(void) const
			{ return x; }
		inline float spur(void) const
			{ return x[0]+x[5]+x[10]+x[15]; }
		inline mat4 transpose(void) const
			{
				return mat4(
					x[ 0], x[ 4], x[ 8], x[12],
					x[ 1], x[ 5], x[ 9], x[13],
					x[ 2], x[ 6], x[10], x[14],
					x[ 3], x[ 7], x[11], x[15]);
			}
		inline vec4 rowvec(int row) const	// row in [0..3]
			{ row *= 4; return vec4(x[row], x[row+1], x[row+2], x[row+3]); }
		inline vec4 colvec(int col) const	// col in [0..3]
			{ return vec4(x[col], x[col+4], x[col+8], x[col+12]); }
		inline mat4 inv(void) const
			{
				float d = det();
				if ((d < 1.0e-7) && (d > -1.0e-7)) return mat4();
				mat4 m(*this), I;
				int i0, i1;
				for (int j = 0; j < 4; ++j) {
					// Diagonalfeld normalisieren
					float q = m.x[j*4+j];
					if (q == 0.0) {
						// Es darf keine 0 mehr in der Diagonalen stehen
						for (int i = j+1; i < 4; ++i) {
							// Suche Reihe mit einem Feld != 0.0 und addiere
							if (m.x[i*4+j] != 0.0) {
								for (int k = 0; k < 4; ++k) {
									i0 = j*4+k; i1 = i*4+k;
									m.x[i0] += m.x[i1];
									I.x[i0] += I.x[i1];
								}
								q = m.x[j*4+j];
								break;
							}
						}
					} else {
						// Diagonalen auf 1 bringen
						for (int k = 0; k < 4; ++k) {
							i0 = j*4+k;
							m.x[i0] /= q;
							I.x[i0] /= q;
						}
					}
					// Spalten ausserhalb der Diagonalen auf 0 bringen
					for (int i = 0; i < 4; ++i) {
						if (i != j) {
							q = m.x[i*4+j];
							for (int k = 0; k < 4; ++k) {
								i0 = i*4+k; i1 = j*4+k;
								m.x[i0] -= q * m.x[i1];
								I.x[i0] -= q * I.x[i1];
							}
						}
					}
				}
				// Test auf Einheitsmatrix, falls nicht => Berechnung nicht gelungen
				for (int i = 0; i < 4; ++i) 
					for (int j = 0; j < 4; ++j)
						if (m.x[i*4+j] != ((i==j) ? 1:0)) return mat4();
				return I;
			}
		friend mat4 operator * (float f, const mat4 & m)
			{ return mat4(m) *= f; }
		friend mat4 operator + (const mat4 & a, const mat4 & b)
			{ return mat4(a) += b; }
		friend mat4 operator - (const mat4 & a, const mat4 & b)
			{ return mat4(a) -= b; }
		friend mat4 operator * (const mat4 & a, const mat4 & b)
			{ return mat4(a) *= b; }
		friend mat4 operator * (const vec4 & v, const mat4 & m)
			{
				float c1 = m.x[ 0]+m.x[ 4]+m.x[ 8]+m.x[12];
				float c2 = m.x[ 1]+m.x[ 5]+m.x[ 9]+m.x[13];
				float c3 = m.x[ 2]+m.x[ 6]+m.x[10]+m.x[14];
				float c4 = m.x[ 3]+m.x[ 7]+m.x[11]+m.x[15];
				return mat4(
					v[0]*c1, v[0]*c2, v[0]*c3, v[0]*c4,
					v[1]*c1, v[1]*c2, v[1]*c3, v[1]*c4,
					v[2]*c1, v[2]*c2, v[2]*c3, v[2]*c4,
					v[3]*c1, v[3]*c2, v[3]*c3, v[3]*c4);
			}
		friend vec4 operator * (const mat4 & m, const vec4 & v)
			{
				return vec4(
					m.x[ 0]*v[0] + m.x[ 1]*v[1] + m.x[ 2]*v[2] + m.x[ 3]*v[3],
					m.x[ 4]*v[0] + m.x[ 5]*v[1] + m.x[ 6]*v[2] + m.x[ 7]*v[3],
					m.x[ 8]*v[0] + m.x[ 9]*v[1] + m.x[10]*v[2] + m.x[11]*v[3],
					m.x[12]*v[0] + m.x[13]*v[1] + m.x[14]*v[2] + m.x[15]*v[3]);
			}
	private:
		float x[16];
};

template <unsigned int N> class matn	// N >= 1
{
	public:
		matn(void)
			{ load_identity(); }
		matn(const matn<N> & m)
			{ for (unsigned int i=0; i<N*N; ++i) x[i]=m.x[i]; }
		matn(float m[])
			{ for (unsigned int i=0; i<N*N; ++i) x[i]=m[i]; }
		inline void load_identity(void)
			{ for (unsigned int i=0; i<N*N; ++i) x[i]=(i%(N+1))?0.0:1.0; }
		inline matn<N> & operator *= (float f)
			{ for (unsigned int i=0; i<N*N; ++i) x[i]*=f; return *this; }
		inline matn<N> & operator += (const matn<N> & m)
			{ for (unsigned int i=0; i<N*N; ++i) x[i]+=m.x[i]; return *this; }
		inline matn<N> & operator -= (const matn<N> & m)
			{ for (unsigned int i=0; i<N*N; ++i) x[i]-=m.x[i]; return *this; }
		inline float spur(void) const
			{ float s = 0.0; for (unsigned int i=0; i<N; ++i) s+=x[i*N+i]; return s; }
		inline matn<N> transpose(void) const
			{
				matn<N> r;
				for (unsigned int i=0; i<N; ++i)
					for (unsigned int j=0; j<N; ++j)
						r.x[i*N+j] = x[j*N+i];
				return r;
			}
		inline matn<N> & operator *= (const matn<N> & m)
			{
				float c[N*N];
				unsigned int idx;
				for (unsigned int i=0; i<N; ++i)
					for (unsigned int j=0; j<N; ++j) {
						idx = i*N+j;
						c[idx] = 0.0;
						for (unsigned int k=0; k<N; ++k) c[idx] += x[i*N+k]*m.x[k*N+j];
					}
				for (unsigned int i=0; i<N*N; ++i) x[i]=((c[i]<1.0e-10)&&(c[i]>-1.0e-10))?0.0:c[i];
				return *this;
			}
		inline float det(void) const
			{	// Regel von Sarrus
				float r = 0.0;
				float ap, am;
				unsigned int k;
				for (unsigned int j=0; j<N; ++j)
				{
					ap = am = 1.0;
					for (unsigned int i=0; i<N; ++i)
					{
						k = (j+i)%N;
						ap *= x[i*N+k];
						am *= x[(N-1-i)*N+k];
					}
					r += ap - am;
				}
				return r;
			}
		inline matn<N> inv(void) const
			{
				float d = det();
				if ((d < 1.0e-7) && (d > -1.0e-7)) return matn<N>();
				matn<N> m(*this), I;
				unsigned int i0, i1;
				float q;
				for (unsigned int j = 0; j < N; ++j)
				{
					// Diagonalfeld normalisieren
					q = m.x[j*N+j];
					if (q == 0.0) {
						// Es darf keine 0 mehr in der Diagonalen stehen
						for (unsigned int i = j+1; i < N; ++i) {
							// Suche Reihe mit einem Feld != 0.0 und addiere
							if (m.x[i*N+j] != 0.0) {
								for (unsigned int k = 0; k < N; ++k) {
									i0 = j*N+k; i1 = i*N+k;
									m.x[i0] += m.x[i1];
									I.x[i0] += I.x[i1];
								}
								q = m.x[j*N+j];
								break;
							}
						}
					} else { // Diagonalen auf 1 bringen
						for (unsigned int k = 0; k < N; ++k) {
							i0 = j*N+k;
							m.x[i0] /= q;
							I.x[i0] /= q;
						}
					}
					// Spalten ausserhalb der Diagonalen auf 0 bringen
					for (unsigned int i = 0; i < N; ++i) {
						if (i != j) {
							q = m.x[i*N+j];
							for (unsigned int k = 0; k < N; ++k) {
								i0 = i*N+k; i1 = j*N+k;
								m.x[i0] -= q * m.x[i1];
								I.x[i0] -= q * I.x[i1];
							}
						}
					}
				}
				// Test auf Einheitsmatrix, falls nicht => Berechnung nicht gelungen
				for (unsigned int i = 0; i < N; ++i)
					for (unsigned int j = 0; j < N; ++j)
						if (m.x[i*N+j] != ((i==j) ? 1:0)) return matn<N>();
				return I;
			}
		inline vecn<N> rowvec(int row) const	// row in [0..N-1]
			{ vecn<N> v; row*=N; for (unsigned int i=0; i<N; ++i) v[i]=x[row+i]; return v; }
		inline vecn<N> colvec(int col) const	// col in [0..N-1]
			{ vecn<N> v; for (unsigned int i=0; i<N; ++i) v[i]=x[col+i*N]; return v; }
		friend matn<N> operator * (float f, const matn<N> & m)
			{ return matn<N>(m) *= f; }
		friend matn<N> operator + (const matn<N> & a, const matn<N> & b)
			{ return matn<N>(a) += b; }
		friend matn<N> operator - (const matn<N> & a, const matn<N> & b)
			{ return matn<N>(a) -= b; }
		friend matn<N> operator * (const matn<N> & a, const matn<N> & b)
			{ return matn<N>(a) *= b; }
		friend matn<N> operator * (const vecn<N> & v, const matn<N> & m)
			{
				matn<N> r;
				vecn<N> c;
				for (unsigned int i=0; i<N; ++i)
					for (unsigned int j=0; j<N; ++j)
						c[i] += m.x[i*N+j];
				for (unsigned int i=0; i<N; ++i)
					for (unsigned int j=0; j<N; ++j)
						r.x[i*N+j] = c[j] * v[i];
				return r;
			}
		friend vecn<N> operator * (const matn<N> & m, const vecn<N> & v)
			{
				vecn<N> r;
				for (unsigned int i=0; i<N; ++i)
					for (unsigned int j=0; j<N; ++j) r[i] += m.x[j+i*N]*v[i];
				return r;
			}
	private:
		float x[N*N];
};

}

#endif
