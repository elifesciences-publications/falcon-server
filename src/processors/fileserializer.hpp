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

/* FileSerializer: serializes data streams to file
 * 
 * input ports:
 * data <IData> (1-256 slots)
 *
 * output ports:
 * none
 *
 * exposed states:
 * none
 *
 * exposed methods:
 * none
 *
 * options:
 * path <string> - server-side path
 * encoding <string> - binary/yaml
 * format <string> - full/nodata/compact
 * overwrite <bool> - overwrite existing file
 * throttle <bool> - throttle saving if we can't keep up
 * throttle_threshold <double> - upstream ringbuffer fill fraction (0-1)
 *   at which throttling takes effect
 * throttle_smooth <double> - smooth level of throttle level (0-1)
 * 
 */

#ifndef FILESERIALIZER_HPP
#define FILESERIALIZER_HPP

#include "../graph/iprocessor.hpp"
#include "../data/serialize.hpp"

class FileSerializer : public IProcessor
{
public:
    virtual void CreatePorts() override;
    virtual void Configure(const YAML::Node& node, const GlobalContext& context) override;
    virtual void Preprocess(ProcessingContext& context) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;

protected:
    void create_preamble( std::ostream & out, int slot );
    
protected:
    PortIn<AnyDataType>* data_port_;
    
    std::string path_;
    std::string encoding_;
    Serialization::Format format_;
    bool overwrite_;
    bool throttle_;
    double throttle_threshold_;
    double throttle_smooth_;
    
    std::unique_ptr<Serialization::Serializer> serializer_;
    std::vector<std::unique_ptr<std::ostream>> streams_;
    std::vector<uint64_t> packetid_;
    std::vector<unsigned int> upstream_buffer_size_;
    
    double throttle_level_;
    std::vector<uint64_t> nskipped_;
    int64_t time_out_us_;

public:
    const std::string DEFAULT_ENCODING = "binary";
    const decltype(format_) DEFAULT_FORMAT = Serialization::Format::FULL;
    const std::string DEFAULT_PATH = "run://";
    const bool DEFAULT_OVERWRITE = false;
    const bool DEFAULT_THROTTLE = false;
    const decltype(throttle_threshold_) DEFAULT_THROTTLE_THRESHOLD = 0.3;
    const decltype(throttle_smooth_) DEFAULT_THROTTLE_SMOOTH = 0.5;
    const decltype(time_out_us_) DEFAULT_TIMEOUT_MICROSEC = 500;
};

#endif //fileserializer.hpp
