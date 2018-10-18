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

#include "environment.hpp"

#include <math.h>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include "utilities/string.hpp"
#include "npyreader/npyreader.h"

constexpr const char* Content::DEFAULT_NAME;

void Content::operator=(const Content& copy) {
    
    id_ = copy.id_;
    name_ = copy.name_;
}

int Content::id() const {
    
    return id_;
}

std::string Content::name() const {
    
    return name_;
}

static void delete_accumulator(Grid* grid);

Content& Grid::identify_content(size_t grid_index) {
    
    if (grid_index < content_nodes[0]) {return contents[0];}
    return contents[1];
}

Grid1D::Grid1D( std::string path_to_grid ) {
    
    dimensions_ = {{0}};

    FILE *grid_file = nullptr;
    if ( (grid_file = fopen(path_to_grid.c_str(), "r")) == nullptr) {
        std::runtime_error("Cannot open grid file.");
    }
    
    uint32_t n_elements = 0;
    if ( get_1D_array_len(grid_file, &n_elements) != 0 ) {
        std::runtime_error(
        "\nCannot read the number of grid elements. Check the size of the NPY array");
    }
    elements_.resize( n_elements );
    auto *elements = get_1D_array_f64(grid_file);
    for ( unsigned int i=0; i<n_elements; ++i ) {
        elements_[i] = elements[i];
    }
    free( elements ); elements = nullptr;
        
    fclose(grid_file); grid_file = nullptr;
}


Grid1D::Grid1D( const Grid1D& copy ) {

    dimensions_ = {{0}};
    elements_.resize( copy.n_elements() );
    
    for ( unsigned int i=0; i<copy.n_elements(); ++i ) {
        elements_[i] = copy.element(i);  
    }
}

std::size_t Grid1D::n_elements() const {
    
    return elements_.size();
}

double* Grid1D::elements() {
    
    return elements_.data();
}

double Grid1D::element( std::size_t grid_index ) const {
    
    return elements_[grid_index];
}

uint16_t* Grid1D::dimensions() {
    
    return static_cast<uint16_t*>( dimensions_.data() );
}

const bool Grid1D::is_valid() const {
    
    if ( n_elements() == 0) { return false;}
    return true;
}

const bool Grid::is_valid() const {
    
    if (this->dimensions == nullptr) {
        return false;
    }
    if (this->elements == nullptr ) {
        return false;
    }
    if (this->n_dim == 0) {
        return false;
    }
    if (this->n_elements == 0) {
        return false;
    }
    return true;
}

int load_grid_from_file(Grid **grid, std::string file_path, uint16_t n_grid_dim,
    Mixture *mixture) {	
    
    FILE *grid_file = nullptr;

    if ( (grid_file = fopen(file_path.c_str(), "r")) == nullptr) {
        perror("Cannot open grid file.");
        return -1;
    }
        
    *grid = (Grid*) malloc(sizeof(Grid));
    if ( get_1D_array_len(grid_file, &(*grid)->n_elements) != 0 ) {
        perror("\nCannot read the number of grid elements. Check the size of the NPY array");
        return -1;
    }
    (*grid)->n_elements = (*grid)->n_elements / n_grid_dim;
    (*grid)->n_dim = n_grid_dim;
    (*grid)->dimensions = (uint16_t *) calloc(n_grid_dim, sizeof(uint16_t));
    (*grid)->elements = get_1D_array_f64(grid_file);
        
    for (unsigned int n=0; n < n_grid_dim; ++ n) {
        (*grid)->dimensions[n] = n;
    }
	
    if (mixture != nullptr) {
        (*grid)->accumulator = (double *) malloc(
            mixture->ncomponents * (*grid)->n_elements * sizeof(double*));
        mixture_prepare_grid_accumulator(mixture, (*grid)->elements,
            (*grid)->n_elements, n_grid_dim,
            (*grid)->dimensions, (*grid)->accumulator);
    } else {
        (*grid)->accumulator = nullptr;
    }
        
    fclose(grid_file); grid_file = nullptr;
    return 0;
}

void create_accumulator(Grid* grid, Mixture* mixture) {
    
    if ( mixture == nullptr ) {
        throw std::runtime_error("nullptr pointer in mixture.");
    };
    if (grid->accumulator != nullptr) {
        std::cerr << "The accumulator for the grid might already exist!" << std::endl;
    } else {
        grid->accumulator = (double *) malloc(
            mixture->ncomponents * grid->n_elements * sizeof(double*));
        mixture_prepare_grid_accumulator(mixture, grid->elements,
            grid->n_elements, grid->n_dim, grid->dimensions, grid->accumulator);
    }
}

void delete_accumulator(Grid* grid) {
 
    if (grid != nullptr) {
        if (grid->accumulator != nullptr) {
            free( grid->accumulator ); grid->accumulator = nullptr;
        }
    }
}

void delete_grid(Grid** grid) {
    
    if ( grid != nullptr) {
        free( (*grid)->dimensions ); (*grid)->dimensions = nullptr;
        free( (*grid)->elements ); (*grid)->elements = nullptr;
        delete_accumulator( *grid );
    }
}

void delete_grid(Grid* grid) {
    
    if ( grid != nullptr) {
        free( grid->dimensions ); grid->dimensions = nullptr;
        free( grid->elements ); grid->elements = nullptr;
        delete_accumulator( grid );
    }
}

Grid copy_grid( Grid* grid ) {
    
    Grid copy;
    
    copy.n_elements = grid->n_elements;
    copy.n_dim = grid->n_dim;
    
    copy.elements = new double [grid->n_elements];
    copy.accumulator = new double [grid->n_elements];
    for ( unsigned int i=0; i<grid->n_elements; ++i ) {
        copy.elements[i] = grid->elements[i];
        copy.accumulator[i] = grid->accumulator[i];
    }
    
    copy.accumulator = new double [grid->n_dim];
    for ( unsigned int i=0; i<grid->n_dim; ++i ) {
        copy.dimensions[i] = grid->dimensions[i];
    }

    copy.content_nodes = grid->content_nodes;
    copy.contents = grid->contents;
    
    return copy;
    
}

Environment::Environment( std::string file_path ) {

    if ( not path_exists( file_path ) ) {
        throw std::runtime_error(
            "The following path to environment definition folder does not exist: "
            + file_path );
    }
    load_environment( file_path );
}

void Environment::load_environment( std::string file_path ) {
    
    if ( load_grid_from_file( &grid, file_path + GRID_FILENAME, 1, nullptr) != 0 ) {
        throw std::runtime_error("Cannot load grid from " + file_path);
    }
    load_content_nodes_from_file( file_path + NODES_FILENAME );
    define_contents();
}

void Environment::load_content_nodes_from_file( std::string filename ) {
    
    if ( not path_exists( filename ) ) {
        throw std::runtime_error(
            "The following path to content nodes file does not exist: "
            + filename );
    }
    
    FILE *node_file = nullptr;
    if ( (node_file = fopen(filename.c_str(), "r")) == nullptr ) {
        throw std::runtime_error("Cannot open the nodes file!");
    }
    
    uint32_t n_columns = 0;
    if ( get_2D_matrix_shape( node_file, &n_contents, &n_columns ) != 0 ) {
        throw std::runtime_error("Cannot read shape of content node matrix");
    }
    if ( n_columns != 2 ) {
        throw std::runtime_error(
            "Unexpected number of columns in the content node matrix");
    }
    
    nodes_ = get_2D_matrix_int32( node_file );
    
    // TODO: check integrity of the matrix (no overlaps)
    
    fclose( node_file ); node_file = nullptr;
}

Environment::~Environment() {
    
    delete_accumulator( grid );
    delete_grid( &grid ); grid = nullptr;
    for (unsigned int i=0; i < n_contents; i ++) {
        free(nodes_[i]); nodes_[i] = nullptr;
    }
    free(nodes_); nodes_ = nullptr;
}

void Environment::define_contents() {
    
    assert( grid != nullptr);
    assert( nodes_ != nullptr);
    unsigned int id = 0;
    for (unsigned int c=0; c < n_contents; c ++) {
        id = c + 1;
        contents_.push_back( Content( id, "arm" + std::to_string(id) ) );
    }
    undefined_content_ = Content( -1, "undefined" );
}

Content& Environment::identify_content( int grid_index ) {
    
    for (unsigned int c=0; c < n_contents; c ++) {
        if ( (grid_index >= nodes_[c][0]) && (grid_index < nodes_[c][1]) ) {
            return contents_[c];
        }
    }
    return undefined_content_;
}

