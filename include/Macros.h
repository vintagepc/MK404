/*

    Macros.h - convenience macros for working with SimAVR.

*/

#ifndef __MACROS_H__
#define __MACROS_H__

// There's nothing here. Avoid macros if possible, they may not be typesafe. 
// Prefer things like (static) inlines, constexprs, and whatnot. Sometimes you have no choice.
// If that's the case, add it here.

#define GL_DEBUG 1
#if GL_DEBUG
    #define GL_CHK_ERR(_w) _w; { int e = glGetError(); if (e) printf("GL Error on %s :: %d: %d\n", __FILE__,  __LINE__ ,e); };
#else
    #define GL_CHK_ERR(_w) _w
#endif

#endif