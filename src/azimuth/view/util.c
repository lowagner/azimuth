/*=============================================================================
| Copyright 2012 Matthew D. Steele <mdsteele@alum.mit.edu>                    |
|                                                                             |
| This file is part of Azimuth.                                               |
|                                                                             |
| Azimuth is free software: you can redistribute it and/or modify it under    |
| the terms of the GNU General Public License as published by the Free        |
| Software Foundation, either version 3 of the License, or (at your option)   |
| any later version.                                                          |
|                                                                             |
| Azimuth is distributed in the hope that it will be useful, but WITHOUT      |
| ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       |
| FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   |
| more details.                                                               |
|                                                                             |
| You should have received a copy of the GNU General Public License along     |
| with Azimuth.  If not, see <http://www.gnu.org/licenses/>.                  |
=============================================================================*/

#include "azimuth/view/util.h"

#include <GL/gl.h>

#include "azimuth/util/color.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

void az_gl_color(az_color_t color) {
  glColor4ub(color.r, color.g, color.b, color.a);
}

void az_gl_rotated(double radians) {
  glRotated(AZ_RAD2DEG(radians), 0, 0, 1);
}

void az_gl_translated(az_vector_t v) {
  glTranslated(v.x, v.y, 0);
}

void az_gl_vertex(az_vector_t v) {
  glVertex2d(v.x, v.y);
}

/*===========================================================================*/
