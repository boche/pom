
//////////////////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify         //
// it under the terms of the version 3 of the GNU General Public License        //
// as published by the Free Software Foundation.                                //
//                                                                              //
// This program is distributed in the hope that it will be useful, but          //
// WITHOUT ANY WARRANTY; without even the implied warranty of                   //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU             //
// General Public License for more details.                                     //
//                                                                              //
// You should have received a copy of the GNU General Public License            //
// along with this program. If not, see <http://www.gnu.org/licenses/>.         //
//                                                                              //
// Written by Francois Fleuret                                                  //
// (C) Ecole Polytechnique Federale de Lausanne                                 //
// Contact <pom@epfl.ch> for comments & bug reports                             //
//////////////////////////////////////////////////////////////////////////////////

#include <cmath>

#include "room.h"
#include "misc.h"

Room::Room(int nb_cameras, int nb_positions) {
  _nb_cameras = nb_cameras;
  _nb_positions = nb_positions;
  _rectangles = new TiltedRec[_nb_cameras * _nb_positions];
  _view_height = new int[nb_cameras];
  _view_width = new int[nb_cameras];
}

Room::~Room() {
  delete[] _rectangles;
}

void Room::save_stochastic_view(char *name,
                                int n_camera,
                                const ProbaView *view,
                                const Vector<scalar_t> *proba_presence) const  {

  RGBImage image(view->get_width(), view->get_height());

  Array<scalar_t> proba_pixel_off(_view_width[n_camera], _view_height[n_camera]);

  for(int px = 0; px < _view_width[n_camera]; px++) for(int py = 0; py < _view_height[n_camera]; py++)
    proba_pixel_off(px, py) = 1.0;

  Array<bool> dots(_view_width[n_camera], _view_height[n_camera]);
  dots.clear();

  for(int n = 0; n < nb_positions(); n++) {
    TiltedRec *r = avatar(n_camera, n);
    if(r->visible) {
      for(int irec = 0; irec < r->stripes.size(); irec++)
        for(int py = r->stripes[irec].ymin; py <= r->stripes[irec].ymax; py++)
          for(int px = r->stripes[irec].xmin; px <= r->stripes[irec].xmax; px++)
            proba_pixel_off(px, py) *= (1 - (*proba_presence)[n]);
      dots(r->xground, r->yground) = true;
//      if(r->xmin > 0 && r->xmax < _view_width-1 && r->ymax < _view_height-1)
//        dots((r->xmax + r->xmin)/2, r->ymax) = true;
    }
  }

  for(int py = 0; py < _view_height[n_camera]; py++) for(int px = 0; px < _view_width[n_camera]; px++) {
    scalar_t r, g, b;
    scalar_t a = proba_pixel_off(px, py);

    if(dots(px, py)) { r = 0.0; g = 0.0; b = 0.0; }
    else {
      if(a < 0.5) { r = 0; g = 0; b = 2*a; }
      else        { r = (a - 0.5) * 2; g = (a - 0.5) * 2; b = 1.0; }
    }

    scalar_t c = (*view)(px, py);

    r = c * 0.0 + (1 - c) * r;
    g = c * 0.8 + (1 - c) * g;
    b = c * 0.6 + (1 - c) * b;

    image.set_pixel(px, py, (unsigned char) (255 * r), (unsigned char) (255 * g), (unsigned char) (255 * b));
  }

  image.write_png(name);
}
