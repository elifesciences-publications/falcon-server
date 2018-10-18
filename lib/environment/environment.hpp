// ---------------------------------------------------------------------
// This file is part of falcon-server.
// 
// Copyright (C) 2015, 2016, 2017 Neuro-Electronics Research Flanders
// 
// Falcon-server is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Falcon-server is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with falcon-server. If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------

#ifndef ENVIRONMENT_HPP
#define	ENVIRONMENT_HPP

#include "gmm/mixture.h"

#include <string>
#include <vector>
#include <array>

const double DEFAULT_PIXEL_TO_CM = 2;

class Content {
    
public:
    Content(int id = DEFAULT_ID, std::string name = DEFAULT_NAME):
    id_(id), name_(name) {}
    
    void operator=(const Content& copy);
    
    int id() const;
    std::string name() const;
    
    inline friend bool operator==(Content &c1, Content &c2) {
        
        return c1.id() == c2.id();
    }
    
private:
    static const int DEFAULT_ID = -1;
    static constexpr const char* DEFAULT_NAME = "undefined";

private:    
    int id_;
    std::string name_;
    
};

class Grid1D {

public: 
    Grid1D( std::string path_to_grid );
    Grid1D( Grid1D const& copy );
    
    std::size_t n_elements() const;
    double* elements();
    double element( std::size_t grid_index ) const;
    uint16_t* dimensions();
    
    const bool is_valid() const;
    
protected:
    std::vector<double> elements_;
    std::array<uint16_t, 1> dimensions_;
};

struct Grid {
    double *elements;
    uint32_t n_elements;
    uint16_t n_dim;
    uint16_t *dimensions;
    double *accumulator;
    std::vector<size_t> content_nodes;
    std::vector<Content> contents;
    
    Content& identify_content(size_t grid_index);
    
    const bool is_valid() const;
        
};

int load_grid_from_file( // return 0 upon success
    Grid **grid,
    std::string file_path,
    uint16_t n_grid_dim,
    Mixture *mixture);

void create_accumulator(Grid* grid, Mixture* mixture);

void delete_grid(Grid** grid);
void delete_grid(Grid* grid);

Grid copy_grid( Grid* grid );

// Loaded from a NPY file containing a 1D array,
// the environment is composed of a grid defined in linearized positions, loaded
// from  NPY file containing a 1D array and a list of contents;
//  a matrix of nodes is used to link the contents to the grid
class Environment {

public:
    // file_path leads to a folder with a file name grid.npy and a file named
    // content_nodes.npy
    Environment( std::string file_path );
    ~Environment();
    
    Content& identify_content(int grid_index);
    
    void load_environment( std::string);
    
    // to be made protected
    uint32_t n_contents;
    Grid* grid;
    std::vector<Content> contents_;
    
private:
    void load_content_nodes_from_file( std::string filename );
    void define_contents();
    
private:
    int32_t** nodes_;
    Content undefined_content_;
    
public:
    std::string GRID_FILENAME = "grid.npy";
    std::string NODES_FILENAME = "content_nodes.npy";
};


#endif	// environment.hpp

