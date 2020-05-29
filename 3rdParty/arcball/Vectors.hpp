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

#ifndef SimpleArcballCamera_Vectors_hpp
#define SimpleArcballCamera_Vectors_hpp

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#ifndef EPSILON
#define EPSILON (1.0e-9)
#endif

namespace math {

class vec2
{
	public:
		enum AXIS { X = 0, Y };
		enum ANGLE { PHI = 0 };
	public:
		vec2(float x = 0.0, float y = 0.0)
			{ this->x[0]=x; this->x[1]=y; }
		vec2(const vec2 & v)
			{ *this = v; }
		vec2(float * v)
			{ x[0]=v[0]; x[1]=v[1]; }
		inline vec2 & set(float a, float b)
			{ x[0]=a; x[1]=b; return *this; }
		inline vec2 & setCircle(float r, float phi_deg)
			{
				phi_deg *= (M_PI / 180.0);
				x[0] = r * cos(phi_deg);
				x[1] = r * sin(phi_deg);
				return *this;
			}
		inline float dot(const vec2 & v) const
			{ return x[0]*v.x[0] + x[1]*v.x[1]; }
		inline float length() const
			{ return sqrt(length2()); }
		inline float length2() const
			{ return x[0]*x[0] + x[1]*x[1]; }
		inline vec2 & normalize(float len = 1.0)
			{
				float l = length();
				return (l != 0.0) ? (*this *= (len / l)) : (*this);
			}
		inline vec2 & rot(float angle_deg)
			{
				angle_deg *= M_PI / 180.0;
				float c = cos(angle_deg);
				float s = sin(angle_deg);
				return *this = vec2(x[0] * c - x[1] * s, x[0] * s + x[1] * c);
			}
		inline float operator [] (int idx) const
			{ return x[idx]; }
		inline vec2 & operator = (const vec2 & v)
			{ x[0]=v.x[0]; x[1]=v.x[1]; return *this; }
		inline bool operator == (const vec2 & v)
			{ return ((x[0]==v.x[0]) && (x[1]==v.x[1])); }
		inline vec2 & operator += (const vec2 & v)
			{ x[0]+=v.x[0]; x[1]+=v.x[1]; return *this; }
		inline vec2 & operator -= (const vec2 & v)
			{ x[0]-=v.x[0]; x[1]-=v.x[1]; return *this; }
		inline vec2 & operator *= (float f)
			{ x[0]*=f; x[1]*=f; return *this; }
		friend vec2 operator + (const vec2 & w, const vec2 & v)
			{ return vec2(w) += v; }
		friend vec2 operator - (const vec2 & w, const vec2 & v)
			{ return vec2(w) -= v; }
		friend vec2 operator * (const vec2 & v, float f)
			{ return vec2(v) *= f; }
		friend vec2 operator * (float f, const vec2 & v)
			{ return vec2(v) *= f; }
		friend float operator * (const vec2 & a, const vec2 & b)
			{ return a.dot(b); }
	private:
		float x[2];
};

class vec3
{
	public:
		enum AXIS { X = 0, Y, Z };
		enum ANGLE { PHI = 0, RHO };
	public:
		vec3(float x = 0.0, float y = 0.0, float z = 0.0)
			{ this->x[0]=x; this->x[1]=y; this->x[2]=z; }
		vec3(const vec3 & v)
			{ *this = v; }
		vec3(float * v)
			{ x[0]=v[0]; x[1]=v[1]; x[2]=v[2]; }
		inline vec3 & set(float a, float b, float c)
			{ x[0]=a; x[1]=b; x[2]=c; return *this; }
		inline vec3 & setSphere(float r, float phi_deg, float rho_deg)
			{
				phi_deg *= (M_PI / 180.0);
				rho_deg *= (M_PI / 180.0);
				x[0] = r * cos(phi_deg) * cos(rho_deg);
				x[1] = r * cos(phi_deg) * sin(rho_deg);
				x[2] = r * sin(phi_deg);
				return *this;
			}
		inline float dot(const vec3 & v) const
			{ return x[0]*v.x[0] + x[1]*v.x[1] + x[2]*v.x[2]; }
		inline vec3 cross(const vec3 & v) const
			{
				return vec3(
					x[1] * v.x[2] - x[2] * v.x[1],
					x[2] * v.x[0] - x[0] * v.x[2],
					x[0] * v.x[1] - x[1] * v.x[0]);
			}
		inline vec3 & nullify(void)
			{
				x[0] = ((x[0] <= EPSILON) && (x[0] >= -EPSILON)) ? 0.0 : x[0];
				x[1] = ((x[1] <= EPSILON) && (x[1] >= -EPSILON)) ? 0.0 : x[1];
				x[2] = ((x[2] <= EPSILON) && (x[2] >= -EPSILON)) ? 0.0 : x[2];
				return *this;
			}
		inline float length() const
			{ return sqrt(length2()); }
		inline float length2() const
			{ return x[0]*x[0] + x[1]*x[1] + x[2]*x[2]; }
		inline vec3 & normalize(float len = 1.0)
			{
				float l = length();
				if ((l < 1.0e-7) && (l > -1.0e-7)) return *this;
				l = len / l;
				x[0]*=l; x[1]*=l; x[2]*=l;
				return *this;
			}
		inline float getSphereR() const
			{ return length(); }
		inline float getSpherePhi() const
			{
				float a = sqrt(x[0]*x[0] + x[1]*x[1]);
				return (a != 0.0) ? (atan(x[2] / a) * 180.0 / M_PI) : 0.0;
			}
		inline float getSphereRho() const
			{
				if (x[0] >= -EPSILON && x[0] <= EPSILON)
				{
					if (x[1] >= -EPSILON && x[1] <= EPSILON) return 0.0;
					else if (x[1] > 0.0) return 90.0;
					else if (x[1] < 0.0) return 270.0;
				}
				if (x[0] > 0.0)
				{
					return atan(x[1] / x[0]) * 180.0 / M_PI;
				}
				else // (x[0] < 0.0)
				{
					return 180.0 + atan(x[1] / x[0]) * 180.0 / M_PI;
				}
			}
		inline vec3 & rot(AXIS axis, float angle_deg)
			{
				switch (axis)
				{
					case X: return rotX(angle_deg);
					case Y: return rotY(angle_deg);
					case Z: return rotZ(angle_deg);
				}
				return *this;
			}
		inline vec3 & rotX(float angle_deg)
			{
				angle_deg *= M_PI / 180.0;
				float c = cos(angle_deg);
				float s = sin(angle_deg);
				return *this = vec3(x[0], x[1]*c - x[2]*s, x[1]*s + x[2]*c);
			}
		inline vec3 & rotY(float angle_deg)
			{
				angle_deg *= M_PI / 180.0;
				float c = cos(angle_deg);
				float s = sin(angle_deg);
				return *this = vec3(x[0]*c - x[2]*s, x[1], x[0]*s + x[2]*c);
			}
		inline vec3 & rotZ(float angle_deg)
			{
				angle_deg *= M_PI / 180.0;
				float c = cos(angle_deg);
				float s = sin(angle_deg);
				return *this = vec3(x[0]*c - x[1]*s, x[0]* s + x[1]*c, x[2]);
			}
		inline vec3 & rotSphere(ANGLE angle, float angle_deg)
			{
				switch (angle)
				{
					case PHI: return rotPhi(angle_deg);
					case RHO: return rotRho(angle_deg);
				}
				return *this;
			}
		inline vec3 & rotPhi(float angle_deg)
			{
				return setSphere(getSphereR(),
					getSpherePhi() + angle_deg, getSphereRho());
			}
		inline vec3 & rotRho(float angle_deg)
			{
				return setSphere(getSphereR(),
					getSpherePhi(), getSphereRho() + angle_deg);
			}
		inline float operator [] (int idx) const
			{ return x[idx]; }
		inline float & operator [] (int idx)
			{ return x[idx]; }
		inline operator const float * (void) const
			{ return x; }
		inline vec3 & operator = (const vec3 & v)
			{ x[0]=v.x[0]; x[1]=v.x[1]; x[2]=v.x[2]; return *this; }
		inline bool operator == (const vec3 & v)
			{ return ((x[0]==v.x[0]) && (x[1]==v.x[1]) && (x[2]==v.x[2])); }
		inline vec3 & operator += (const vec3 & v)
			{ x[0]+=v.x[0]; x[1]+=v.x[1]; x[2]+=v.x[2]; return *this; }
		inline vec3 & operator -= (const vec3 & v)
			{ x[0]-=v.x[0]; x[1]-=v.x[1]; x[2]-=v.x[2]; return *this; }
		inline vec3 & operator *= (float f)
			{ x[0]*=f; x[1]*=f; x[2]*=f; return *this; }
		friend vec3 operator + (const vec3 & w, const vec3 & v)
			{ return vec3(w) += v; }
		friend vec3 operator - (const vec3 & w, const vec3 & v)
			{ return vec3(w) -= v; }
		friend vec3 operator * (const vec3 & v, float f)
			{ return vec3(v) *= f; }
		friend vec3 operator * (float f, const vec3 & v)
			{ return vec3(v) *= f; }
		friend float operator * (const vec3 & a, const vec3 & b)
			{ return a.dot(b); }
	private:
		float x[3];
};

class vec4
{
	public:
		enum AXIS { X = 0, Y, Z, W };
		enum ANGLE { PHI = 0, RHO };
	public:
		vec4(float x = 0.0, float y = 0.0, float z = 0.0, float w = 0.0)
			{ this->x[0]=x; this->x[1]=y; this->x[2]=z; this->x[3]=w; }
		vec4(const vec4 & v)
			{ *this = v; }
		vec4(float * v)
			{ x[0]=v[0]; x[1]=v[1]; x[2]=v[2]; x[3]=v[3]; }
		inline vec4 & set(float a, float b, float c, float d)
			{ x[0]=a; x[1]=b; x[2]=c; x[3]=d; return *this; }
		inline float dot(const vec4 & v) const
			{ return x[0]*v.x[0] + x[1]*v.x[1] + x[2]*v.x[2] + x[3]*v.x[3]; }
		inline float length() const
			{ return sqrt(length2()); }
		inline float length2() const
			{ return x[0]*x[0] + x[1]*x[1] + x[2]*x[2] + x[3]*x[3]; }
		inline vec4 & normalize(float len = 1.0)
			{
				float l = length();
				return (l != 0.0) ? (*this *= (len / l)) : (*this);
			}
		inline float operator [] (int idx) const
			{ return x[idx]; }
		inline operator const float * (void) const
			{ return x; }
		inline vec4 & operator = (const vec4 & v)
			{ x[0]=v.x[0]; x[1]=v.x[1]; x[2]=v.x[2]; x[3]=v.x[3]; return *this; }
		inline bool operator == (const vec4 & v)
			{ return ((x[0]==v.x[0]) && (x[1]==v.x[1]) && (x[2]==v.x[2]) && (x[3]==v.x[3])); }
		inline vec4 & operator += (const vec4 & v)
			{ x[0]+=v.x[0]; x[1]+=v.x[1]; x[2]+=v.x[2]; x[3]+=v.x[3]; return *this; }
		inline vec4 & operator -= (const vec4 & v)
			{ x[0]-=v.x[0]; x[1]-=v.x[1]; x[2]-=v.x[2]; x[3]-=v.x[3]; return *this; }
		inline vec4 & operator *= (float f)
			{ x[0]*=f; x[1]*=f; x[2]*=f; x[3]*=f; return *this; }
		friend vec4 operator + (const vec4 & w, const vec4 & v)
			{ return vec4(w) += v; }
		friend vec4 operator - (const vec4 & w, const vec4 & v)
			{ return vec4(w) -= v; }
		friend vec4 operator * (const vec4 & v, float f)
			{ return vec4(v) *= f; }
		friend vec4 operator * (float f, const vec4 & v)
			{ return vec4(v) *= f; }
		friend float operator * (const vec4 & a, const vec4 & b)
			{ return a.dot(b); }
	private:
		float x[4];
};

template <unsigned int N> class vecn	// N >= 1
{
	public:
		vecn(void)
			{ for (unsigned int i=0; i<N; ++i) x[i]=0.0; }
		vecn(const vecn<N> & v)
			{ for (unsigned int i=0; i<N; ++i) x[i]=v.x[i]; }
		vecn(float * v)
			{ for (unsigned int i=0; i<N; ++i) x[i]=v[i]; }
		inline float dot(const vecn<N> & v) const
			{ float r=0.0; for (unsigned int i=0; i<N; ++i) r+=(x[i]*v.x[i]); return r; }
		inline float length() const
			{ return sqrt(length2()); }
		inline float length2() const
			{ float r=0.0; for (unsigned int i=0; i<N; ++i) r+=(x[i]*x[i]); return r; }
		inline vecn<N> & normalize(float len = 1.0)
			{
				float l = length();
				return ((l < 1.0e-7) && (l > -1.0e-7)) ? (*this *= (len / l)) : (*this);
			}
		inline float operator [] (int idx) const
			{ return x[idx]; }
		inline vecn<N> & operator = (const vecn<N> & v)
			{ for (unsigned int i=0; i<N; ++i) x[i]=v.x[i]; return *this; }
		inline bool operator == (const vecn<N> & v)
			{ for (unsigned int i=0; i<N; ++i) if (x[i]!=v.x[i]) return false; return true; }
		inline vecn<N> & operator += (const vecn<N> & v)
			{ for (unsigned int i=0; i<N; ++i) x[i]+=v.x[i]; return *this; }
		inline vecn<N> & operator -= (const vecn<N> & v)
			{ for (unsigned int i=0; i<N; ++i) x[i]-=v.x[i]; return *this; }
		inline vecn<N> & operator *= (float f)
			{ for (unsigned int i=0; i<N; ++i) x[i]*=f; return *this; }
		friend vecn<N> operator + (const vecn<N> & w, const vecn<N> & v)
			{ return vecn<N>(w) += v; }
		friend vecn<N> operator - (const vecn<N> & w, const vecn<N> & v)
			{ return vecn<N>(w) -= v; }
		friend vecn<N> operator * (const vecn<N> & v, float f)
			{ return vecn<N>(v) *= f; }
		friend vecn<N> operator * (float f, const vecn<N> & v)
			{ return vecn<N>(v) *= f; }
		friend float operator * (const vecn<N> & a, const vecn<N> & b)
			{ return a.dot(b); }
	private:
		float x[N];
};

}

#endif
