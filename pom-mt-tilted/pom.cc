
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

#include <iostream>
#include <fstream>

using namespace std;

#include "misc.h"
#include "global.h"
#include "vector.h"
#include "room.h"
#include "pom_solver.h"
#include <thread>
#include <mutex>

void check_parameter(char *s, int line_number, char *buffer) {
  if(!s) {
    cerr << "Missing parameter line " << line_number << ":" << endl;
    cerr << buffer << endl;
    exit(1);
  }
}

int fmax;
const int THREAD_POOL_SIZE = 8;
mutex COUT_MUTEX, CERR_MUTEX;
char input_view_format[buffer_size] = "";
char result_format[buffer_size] = "";
char result_view_format[buffer_size] = "";
char convergence_view_format[buffer_size] = "";
char buffer[buffer_size], token[buffer_size];
Room *room = 0;
ifstream *configuration_file = 0;
Vector<scalar_t> prior;

void process_frame(int f){
    RGBImage tmp;
    POMSolver solver(room);
    Vector<ProbaView *> *proba_views = 0;
    Vector<scalar_t> proba_presence(room->nb_positions());
    char tmp_path[buffer_size];

    proba_views = new Vector<ProbaView *>(room->nb_cameras());
    for (int c = 0; c < proba_views->length(); c++)
        (*proba_views)[c] = new ProbaView(room->view_width(c), room->view_height(c));

    for (; f < fmax; f += THREAD_POOL_SIZE) {
        if(configuration_file) {
            lock_guard<mutex> lck(COUT_MUTEX);
            cout << "Processing frame " << f << endl;
        }

        for (int c = 0; c < room->nb_cameras(); c++) {
            pomsprintf(tmp_path, buffer_size, input_view_format, c, f, 0);
            tmp.read_png(tmp_path);
            (*proba_views)[c]->from_image(&tmp);
        }

        if (strcmp(convergence_view_format, "") != 0)
            solver.solve(room, &prior, proba_views, &proba_presence, f, convergence_view_format);
        else
            solver.solve(room, &prior, proba_views, &proba_presence, f, 0);

        if (strcmp(result_view_format, "") != 0)
            for (int c = 0; c < room->nb_cameras(); c++) {
                pomsprintf(tmp_path, buffer_size, result_view_format, c, f, 0);
                room->save_stochastic_view(tmp_path, c, (*proba_views)[c], &proba_presence);
            }

        if (strcmp(result_format, "") != 0) {
            pomsprintf(tmp_path, buffer_size, result_format, 0, f, 0);
            ofstream result(tmp_path);
            if (result.fail()) {
                lock_guard<mutex> lck(CERR_MUTEX);
                cerr << "Can not open " << token << " for writing." << endl;
                exit(1);
            }
            for (int i = 0; i < room->nb_positions(); i++)
                result << i << " " << proba_presence[i] << endl;
            result.flush();
        }
    }
    if (proba_views)
        for (int c = 0; c < proba_views->length(); c++) delete (*proba_views)[c];
    delete proba_views;
}

int main(int argc, char **argv) {

    if (argc > 2) {
        cerr << argv[0] << " [-h | --help | <configuration file>]" << endl;
        exit(1);
    }

    istream *input_stream;

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            cout << argv[0] << " [-h | --help | <configuration file>]" << endl
            << endl
            << "  If a configuration file name is provided, the programs processes it" << endl
            << "  and prints information about the files it generates. Otherwise, it" << endl
            << "  reads the standard input and does not produce any output unless an" << endl
            << "  error occurs." << endl
            << endl;
            exit(0);
        }
        configuration_file = new ifstream(argv[1]);
        if (configuration_file->fail()) {
            cerr << "Can not open " << argv[1] << " for reading." << endl;
            exit(1);
        }
        input_stream = configuration_file;
    } else input_stream = &cin;

    int line_number = 0;
    while (!input_stream->eof()) {

        input_stream->getline(buffer, buffer_size);
        line_number++;

        char *s = buffer;
        s = next_word(token, s, buffer_size);

        if (strcmp(token, "ROOM") == 0) {
            int nb_positions = -1;
            int nb_cameras = -1;

            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            nb_cameras = atoi(token);

            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            nb_positions = atoi(token);

            if (room) {
                cerr << "Room already defined, line" << line_number << "." << endl;
                exit(1);
            }
            room = new Room(nb_cameras, nb_positions);
        }
        else if(strcmp(token, "CAMERA") == 0){
            int view_width = -1, view_height = -1, n_camera = -1;
            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            n_camera = atoi(token);

            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            view_width = atoi(token);

            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            view_height = atoi(token);
            room->set_camera_size(n_camera, view_height, view_width);
        }
        else if (strcmp(token, "CONVERGENCE_VIEW_FORMAT") == 0) {
            check_parameter(s, line_number, buffer);
            s = next_word(convergence_view_format, s, buffer_size);
        }

        else if (strcmp(token, "INPUT_VIEW_FORMAT") == 0) {
            check_parameter(s, line_number, buffer);
            s = next_word(input_view_format, s, buffer_size);
        }

        else if (strcmp(token, "RESULT_VIEW_FORMAT") == 0) {
            check_parameter(s, line_number, buffer);
            s = next_word(result_view_format, s, buffer_size);
        }

        else if (strcmp(token, "RESULT_FORMAT") == 0) {
            check_parameter(s, line_number, buffer);
            s = next_word(result_format, s, buffer_size);
        }

        else if (strcmp(token, "PROCESS") == 0) {
            int first_frame, nb_frames;

            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            first_frame = atoi(token);

            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            nb_frames = atoi(token);
            fmax = first_frame+nb_frames;

            prior.resize(room->nb_positions());
            for (int i = 0; i < room->nb_positions(); i++)
                prior[i] = global_prior;
            if (strcmp(input_view_format, "") == 0) {
                cerr << "You must specify the input view format." << endl;
                exit(1);
            }

            thread tarray[THREAD_POOL_SIZE];
            for(int t = 0; t < THREAD_POOL_SIZE; t++)
                tarray[t] = thread(process_frame, first_frame + t);
            for(int t = 0; t < THREAD_POOL_SIZE; t++)
                tarray[t].join();
        }

        else if (strcmp(token, "TILTEDREC") == 0) {
            int n_camera, n_position;

            if (!room) {
                cerr << "You must define a room before adding rectangles, line" << line_number << "." << endl;
                exit(1);
            }

            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            n_camera = atoi(token);

            if (n_camera < 0 || n_camera >= room->nb_cameras()) {
                cerr << "Out of range camera number line " << line_number << "." << endl;
                exit(1);
            }

            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            n_position = atoi(token);

            if (n_position < 0 || n_camera >= room->nb_positions()) {
                cerr << "Out of range position number line " << line_number << "." << endl;
                exit(1);
            }

            TiltedRec *current = room->avatar(n_camera, n_position);

            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            if (strcmp(token, "notvisible") == 0) {
                current->visible = false;
                current->stripes.push_back({-1, -1, -1, -1});
            } else {
                current->visible = true;

                current->xground = atoi(token);
                check_parameter(s, line_number, buffer);
                s = next_word(token, s, buffer_size);
                current->yground = atoi(token);

                check_parameter(s, line_number, buffer);
                s = next_word(token, s, buffer_size);
                int nrec = atoi(token);

                for(int irec = 0; irec < nrec; irec++){
                    check_parameter(s, line_number, buffer);
                    s = next_word(token, s, buffer_size);
                    int xmin = atoi(token);
                    check_parameter(s, line_number, buffer);
                    s = next_word(token, s, buffer_size);
                    int ymin = atoi(token);
                    check_parameter(s, line_number, buffer);
                    s = next_word(token, s, buffer_size);
                    int xmax = atoi(token);
                    check_parameter(s, line_number, buffer);
                    s = next_word(token, s, buffer_size);
                    int ymax = atoi(token);
                    if (xmin < 0 || xmax >= room->view_width(n_camera) ||
                        ymin < 0 || ymax >= room->view_height(n_camera)) {
                        cerr << "TiltedRec out of bounds, line " << line_number << endl;
                        exit(1);
                    }
                    current->stripes.push_back({xmin, ymin, xmax, ymax});
                }
            }
        }

        else if (strcmp(token, "PRIOR") == 0) {
            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            global_prior = atof(token);
        }

        else if (strcmp(token, "SIGMA_IMAGE_DENSITY") == 0) {
            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            global_sigma_image_density = atof(token);
        }

        else if (strcmp(token, "SMOOTHING_COEFFICIENT") == 0) {
            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            global_smoothing_coefficient = atof(token);
        }

        else if (strcmp(token, "MAX_NB_SOLVER_ITERATIONS") == 0) {
            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            global_max_nb_solver_iterations = atoi(token);
        }

        else if (strcmp(token, "ERROR_MAX") == 0) {
            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            global_error_max = atof(token);
        }

        else if (strcmp(token, "NB_STABLE_ERROR_FOR_CONVERGENCE") == 0) {
            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            global_nb_stable_error_for_convergence = atoi(token);
        }

        else if (strcmp(token, "PROBA_IGNORED") == 0) {
            check_parameter(s, line_number, buffer);
            s = next_word(token, s, buffer_size);
            global_proba_ignored = atof(token);
            cout << "global_proba_ignored = " << global_proba_ignored << endl;
        }

        else if (strcmp(buffer, "") == 0 || buffer[0] == '#') { }

        else {
            cerr << "Unknown token " << token << ".";
            exit(1);
        }
    }

    delete room;

    delete configuration_file;
}


