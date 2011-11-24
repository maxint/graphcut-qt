/*
 * GrabCut implementation source code Copyright(c) 2005-2006 Justin Talbot
 *
 * All Rights Reserved.
 * For educational use only; commercial use expressly forbidden.
 * NO WARRANTY, express or implied, for this software.
 */

#ifndef COLOR_H
#define COLOR_H

#include "Global.h"
#include "Image.h"

namespace GrabCutNS {

class Color {

public:

	Color() : r(0), g(0), b(0) {}
	Color(Real _r, Real _g, Real _b) : r(_r), g(_g), b(_b) {}

	Real r, g, b;
};

// Compute squared distance between two colors
Real distance2( const Color& c1, const Color& c2 );

}
#endif //COLOR_H
