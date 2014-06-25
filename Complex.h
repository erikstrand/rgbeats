//==============================================================================
// Complex.hpp
// Created 1/16/12.
//==============================================================================

#ifndef ESTDLIB_COMPLEX
#define ESTDLIB_COMPLEX

#include <cmath>


//==============================================================================
// Class NWrap<T>
//==============================================================================

// Gets the absolute value of (hopefully) any Real number.
template<typename T>
inline T absval (T const& t) { return t.abs(); }
template<> inline unsigned absval<unsigned> (unsigned const& x) { return x; }
template<> inline int      absval<int>      (int const& x)      { return abs(x); }
template<> inline float    absval<float>    (float const& x)    { return abs(x); }
template<> inline double   absval<double>   (double const& x)   { return abs(x); }


//==============================================================================
// Class Complex<T>
//==============================================================================

// Note: Should consider making Complex<T> formally a POD for POD T

template <class T>
class Complex {
private:
   T _re;
   T _im;
   
public:
   // Constructors
   Complex () {}
//   Complex (T real): _re(real), _im(0) {}
   Complex (T real, T imag): _re(real), _im(imag) {}
   
   // Assignment
//   Complex& operator= (T t) {_re = t; _im = 0; return *this;}
   Complex& operator= (Complex const& z) { _re = z.re(); _im = z.im(); return *this; }

   inline static void zero (Complex<T>* data, unsigned n);

   // Basic Access
   T& re ()       { return _re; }
   T& im ()       { return _im; }
   T  re () const { return _re; }
   T  im () const { return _im; }
   
   // Conjugation and Inversion
   T norm () const;
   T normsquare () const { return re()*re() + im()*im(); }
   // the fact that these have the same name could cause problems...
   Complex& conjugate () {_im *= -1; return *this;}
   Complex  conjugate () const {return Complex(_re, -1*_im);}
   inline Complex& invert ();
   inline Complex  inverse () const;
   
   // Field Operations
   Complex operator+ (Complex const& z) const { return Complex(re()+z.re(), im()+z.im()); }
   Complex operator- (Complex const& z) const { return Complex(re()-z.re(), im()-z.im()); }
   inline Complex operator* (Complex const& z) const;
   inline Complex operator/ (Complex const& z) const;
   
   // In-Place Field Operations
   Complex& operator+= (Complex const& z) { return *this = *this + z; }
   Complex& operator-= (Complex const& z) { return *this = *this - z; }
   Complex& operator*= (Complex const& z) { return *this = *this * z; }
   Complex& operator/= (Complex const& z) { return *this = *this / z; }
   
   // Comparisons
   bool operator== (Complex const& z) const { return re()==z.re() and im()==z.im(); }
   bool operator!= (Complex const& z) const { return !(*this == z); }
};

/*
template <class T>
std::ostream& operator<< (std::ostream& os, Complex<T> const& z) {
   os << z.re() << " + " << z.im() << 'i';
   return os;
}
*/


//==============================================================================
// Method Definitions
//==============================================================================

//------------------------------------------------------------------------------
// Assignment Methods
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// General zero method
template <class T>
void Complex<T>::zero (Complex<T>* data, unsigned n) {
   for (unsigned i=0; i<n; ++i) {
    data[i].re() = T(0); data[i].im() = T(0);
  }
}

//------------------------------------------------------------------------------
// Specialization for floats (uses memset)
template <>
inline void Complex<float>::zero (Complex<float>* data, unsigned n) {
  memset(data, 0, n * sizeof(Complex<float>));
}

//------------------------------------------------------------------------------
// Specialization for ints (uses memset)
template <>
inline void Complex<int>::zero (Complex<int>* data, unsigned n) {
  memset(data, 0, n * sizeof(Complex<int>));
}


//------------------------------------------------------------------------------
// Conjugation and Inversion
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Gotta do some fancy footwork to avoid overflow.
template <class T>
T Complex<T>::norm () const {
   T temp;
   if (_re >= _im) {
      temp = _im / _re;
      return absval<T>(_re) * sqrt(1 + temp*temp);
   } else {
      temp = _re / _im;
      return absval<T>(_im) * sqrt(1 + temp*temp);
   }
}

//------------------------------------------------------------------------------
template <class T>
Complex<T>& Complex<T>::invert () {
   T temp1, temp2;
   if (re() >= im()) {
      temp1 = im() / re();
      temp2 = re() + im()*temp1;
      re() = 1/temp2;
      im() = -temp1/temp2;
   } else {
      temp1 = re() / im();
      temp2 = re()*temp1 + im();
      re() = temp1/temp2;
      im() = -1/temp2;
   }
   return *this;
}

//------------------------------------------------------------------------------
template <class T>
Complex<T> Complex<T>::inverse () const {
   T temp1, temp2;
   if (re() >= im()) {
      temp1 = im() / re();
      temp2 = re() + im()*temp1;
      return Complex( 1/temp2, -temp1/temp2 );
   } else {
      temp1 = re() / im();
      temp2 = re()*temp1 + im();
      return Complex( temp1/temp2, -1/temp2 );
   }
}


//------------------------------------------------------------------------------
// Field Operations
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
template <class T>
Complex<T> Complex<T>::operator* (Complex const& z) const {
   return Complex<T>(re()*z.re() - im()*z.im(), re()*z.im() + im()*z.re());
}

//------------------------------------------------------------------------------
template <class T>
Complex<T> Complex<T>::operator/ (Complex const& z) const {
   T temp1, temp2;
   if (z.re() >= z.im()) {
      temp1 = z.im() / z.re();
      temp2 = z.re() + z.im() * temp1;
      return Complex( (re() + im() * temp1) / temp2, (im() - re() * temp1) / temp2 );
   } else {
      temp1 = z.re() / z.im();
      temp2 = z.re() * temp1 + z.im();
      return Complex( (re() * temp1 + im()) / temp2, (im() * temp1 - re()) / temp2 );
   }
}


#endif // ESTDLIB_COMPLEX

