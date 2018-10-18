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

#ifndef FILTER_HPP
#define FILTER_HPP

#include <vector>
#include <array>

#include <fstream>
#include <map>
#include <string>
#include <cstring>
#include <regex>
#include <cassert>

#include <cctype>
#include <algorithm>

#include <exception>

#include "yaml-cpp/yaml.h"

namespace dsp {
namespace filter {

class IFilter {
public:
    IFilter( std::string description ) : description_(description) {}
    
    ~IFilter( );
    
    std::string description() const;
    
    virtual IFilter* clone() = 0;
    
    virtual unsigned int order() const = 0;
    unsigned int nchannels() const;
    
    bool realized() const;
    void realize( unsigned int nchannels, double init = 0.0);
    
    void unrealize();

    // single channel, single sample
    virtual double process_channel( double, unsigned int channel  ) = 0;
    
    // all channels, single sample
    virtual void process_sample( std::vector<double>&, std::vector<double>& ) = 0;
    virtual void process_sample( std::vector<double>::iterator, std::vector<double>::iterator ) = 0;
    virtual void process_sample( double*, double* ) = 0;
    
    // single channel, multiple samples
    virtual void process_channel( std::vector<double> &, std::vector<double> &, unsigned int channel =0 ) = 0;
    virtual void process_channel( uint64_t nsamples, std::vector<double>::iterator, std::vector<double>::iterator, unsigned int channel=0) = 0;
    virtual void process_channel( uint64_t nsamples, double*, double*, unsigned int channel ) = 0;
    
    // all channels, multiple samples
    virtual void process_by_channel( std::vector<std::vector<double>> &, std::vector<std::vector<double>> & ) = 0; // samples<channels>
    virtual void process_by_sample( std::vector<std::vector<double>> &, std::vector<std::vector<double>> & ) = 0; // channels<samples>
    virtual void process_by_channel( uint64_t nsamples, double**, double** ) = 0; // samples<channels>
    virtual void process_by_sample( uint64_t nsamples, double**, double** ) = 0; // channels<samples>
    virtual void process_by_channel( uint64_t nsamples, std::vector<double>&, std::vector<double>& ) = 0; // samples<channels>
    virtual void process_by_sample( uint64_t nsamples, std::vector<double>& , std::vector<double>&) = 0; // channels<samples>

protected:
    virtual bool realize_filter( unsigned int nchannels, double init ) = 0;
    virtual void unrealize_filter() = 0;

protected:
    std::string description_;
    bool realized_ = false;
    unsigned int nchannels_ = 0;
};

class FirFilter : public IFilter {
public:
    FirFilter( std::vector<double> & coefficients, std::string description = "" );
    virtual IFilter* clone();
    
    static FirFilter* FromStream( std::istream & stream, std::string description, bool binary = false );
    
    unsigned int order() const override final;
    
    std::size_t group_delay() const;
    
    // single channel, single sample
    double process_channel( double input, unsigned int channel = 0 ) override final;
    
    // all channels, single sample
    void process_sample( std::vector<double>& input, std::vector<double>& output ) override final;
    void process_sample( std::vector<double>::iterator input, std::vector<double>::iterator output ) override final;
    void process_sample( double* input, double* output ) override final;
    
    // single channel, multiple samples
    void process_channel( std::vector<double> & input, std::vector<double> & output, unsigned int channel=0 ) override final;
    void process_channel( uint64_t nsamples, std::vector<double>::iterator input, std::vector<double>::iterator output, unsigned int channel=0);
    void process_channel( uint64_t nsamples, double* input, double* output, unsigned int channel=0 );
    
    // all channels, multiple samples
    void process_by_channel( std::vector<std::vector<double>> & input, std::vector<std::vector<double>> & output ) override final;
    void process_by_sample( std::vector<std::vector<double>> &input, std::vector<std::vector<double>> &output ) override final;
    void process_by_channel( uint64_t nsamples, double** input, double** output) override final;
    void process_by_sample( uint64_t nsamples, double** input, double** output ) override final;

    virtual void process_by_channel( uint64_t nsamples, std::vector<double>& input, std::vector<double>& output );
    virtual void process_by_sample( uint64_t nsamples, std::vector<double>& input, std::vector<double>& output);

protected:
    virtual bool realize_filter( unsigned int nchannels, double init = 0.0 ) override final;
    void unrealize_filter() override final;

protected:
    std::vector<double> coefficients_;
    double* pcoefficients_;
    unsigned int ntaps_;
    
    std::vector<std::vector<double>> registers_;
    std::vector<double*> pregisters_;
    
    double* preg_;

};

class BiquadFilter : public IFilter {
public:
    BiquadFilter( double gain, std::vector<std::array<double,6>>  & coefficients, std::string description = "" );
    virtual IFilter* clone();
    
    static BiquadFilter* FromStream( std::istream & stream, std::string description, bool binary );
    
    unsigned int order() const override final;
    
    // single channel, single sample
    double process_channel( double x, unsigned int c = 0 ) override final;
    
    // all channels, single sample
    void process_sample( std::vector<double>& input, std::vector<double>& output) override final {};
    void process_sample( std::vector<double>::iterator input, std::vector<double>::iterator output) override final;
    void process_sample( double* input, double* output) override final;
    
    // single channel, multiple samples
    void process_channel( std::vector<double> & input, std::vector<double> & output, unsigned int channel=0 ) override final;
    void process_channel( uint64_t nsamples, std::vector<double>::iterator input, std::vector<double>::iterator output, unsigned int channel=0) override final;
    void process_channel( uint64_t nsamples, double* input, double* output, unsigned int channel=0 ) override final;
    
    // all channels, multiple samples
    void process_by_channel( std::vector<std::vector<double>> & input, std::vector<std::vector<double>> & output ) override final;
    void process_by_sample( std::vector<std::vector<double>> &input, std::vector<std::vector<double>> &output ) override final;
    void process_by_channel( uint64_t nsamples, double** input, double** output) override final;
    void process_by_sample( uint64_t nsamples, double** input, double** output ) override final;
    
    virtual void process_by_channel( uint64_t nsamples, std::vector<double>& input, std::vector<double>& output );
    virtual void process_by_sample( uint64_t nsamples, std::vector<double>& input, std::vector<double>& output);
    
protected:
    virtual bool realize_filter( unsigned int nchannels, double init = 0.0 ) override final;
    void unrealize_filter() override final;

protected:
    double gain_;
    std::vector<std::array<double,6>> coefficients_;
    
    unsigned int nstages_;
    
    std::vector< std::vector< std::array<double,2> > > registers_;
    
};

std::map<std::string,std::string> parse_file_header( std::istream & stream );
IFilter* construct_from_file( std::string file );

IFilter* construct_from_yaml( const YAML::Node & node );

} // namespace filter

} // namespace dsp


namespace YAML {
        
    // std::array
    template <typename T, std::size_t S>
    struct convert<std::array<T,S> > {
      static Node encode(const std::array<T,S>& rhs) {
        Node node(NodeType::Sequence);
        for (typename std::array<T,S>::const_iterator it = rhs.begin();
             it != rhs.end(); ++it)
          node.push_back(*it);
        return node;
      }
    
      static bool decode(const Node& node, std::array<T,S>& rhs) {
        if (!node.IsSequence())
          return false;
        
        if (node.size()!=S)
            return false;
        
        int index = 0; 
        
        for (const_iterator it = node.begin(); it != node.end(); ++it) {
            rhs[index++] = it->as<T>();
        }
        
        return true;
      }
    };

}

#endif // FILTER_HPP
