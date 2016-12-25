#include "fileserializer.hpp"

#include "../data/idata.hpp"
#include "g3log/src/g2log.hpp"
#include "utilities/time.hpp"

#include <fstream>

void FileSerializer::CreatePorts() {
    
    data_port_ = create_input_port(
        "data",
        AnyDataType(),
        PortInPolicy( SlotRange(1,256), false, 0 ) );
}

void FileSerializer::Configure( const YAML::Node & node, const GlobalContext& context ) {
    
    path_ = node["path"].as<std::string>( DEFAULT_PATH );
    
    encoding_ = node["encoding"].as<std::string>( DEFAULT_ENCODING );
    std::transform( encoding_.begin(), encoding_.end(), encoding_.begin(),
        (int (*)(int))std::tolower );
    if ( encoding_ != "binary" and encoding_ !="yaml" ) {
        throw ProcessingConfigureError(
            "Invalid encoding mode. Must be either binary or yaml", name() );
    }
    
    format_ = Serialization::string_to_format( node["format"].as<std::string>(
        Serialization::format_to_string( DEFAULT_FORMAT ) ) );
    
    overwrite_ = node["overwrite"].as<bool>( DEFAULT_OVERWRITE );
    
    throttle_ = node["throttle"].as<bool>( DEFAULT_THROTTLE );
    throttle_threshold_ = node["throttle_threshold"].as<double>(
        DEFAULT_THROTTLE_THRESHOLD );
    if (throttle_threshold_<0 || throttle_threshold_>1) {
        throw ProcessingConfigureError("Throttle threshold should be in range 0-1.", name());
    }
    throttle_smooth_ = node["throttle_smooth"].as<double>( DEFAULT_THROTTLE_SMOOTH );
    if (throttle_smooth_<0 || throttle_smooth_>1) {
        throw ProcessingConfigureError("Throttle smooth value should be in range 0-1.", name());
    }
}

void FileSerializer::Preprocess(ProcessingContext& context) {
    
    // create serializer object
    serializer_.reset( Serialization::serializer_from_string( encoding_, format_ ) );
    
    // initialization
    packetid_.assign( data_port_->number_of_slots(), 0 );
    upstream_buffer_size_.assign( data_port_->number_of_slots(), 0 );
    nskipped_.assign(data_port_->number_of_slots(), 0 );
    throttle_level_ = 0;
    
    // create output file streams
    streams_.clear();
    
    std::string path = context.resolve_path( path_, "run" );
    std::string address;
    std::string filename;
    std::unique_ptr<std::ostream> stream;
    
    for (int k=0; k<data_port_->number_of_slots(); k++) {
        
        address = data_port_->slot(k)->upstream_address().string();
        filename = path + "/" + name() + "." + std::to_string(k) + "_" + address +
            "." + serializer_->extension();
        
        // if file exists
        if (!overwrite_ && path_exists(filename)) {
            throw ProcessingError( "Output file " + filename + " exists.", name() );
        }
        // try to open file
        stream = std::unique_ptr<std::ostream>(
            new std::ofstream( filename, std::ofstream::out | std::ofstream::binary ) );
        
        if (!stream->good()) {
            throw ProcessingError( "Error opening output file " + filename + ".", name() );
        }
        
        create_preamble( *stream.get(), k );
        
        streams_.push_back( std::move(stream) );
        
        LOG(DEBUG) << "Successfully opened output file for stream " <<
            std::to_string(k) << " (" << address << ").";
        
        upstream_buffer_size_[k] = static_cast<unsigned int>(
            data_port_->slot(k)->upstream_policy().buffer_size() );
    }   
        
}

void FileSerializer::create_preamble( std::ostream & out, int slot ) {
    
    YAML::Node node;
    
    node["creator"] = name();
    node["date"] = time_to_string( std::time(nullptr) );
    node["version"] = static_cast<int>( Serialization::VERSION );
    node["interleaved"] = false;
    node["format"] = Serialization::format_to_string(format_);
    node["encoding"] = encoding_;
    
    node["stream"] = slot;
    
    node["data"] = serializer_->DataDescription( data_port_->slot(slot)->GetDataPrototype() );
    
    YAML::Emitter emit( out );
    emit << YAML::BeginDoc;
    emit << node;
    emit << YAML::EndDoc;
    
}


void FileSerializer::Process(ProcessingContext& context) {
      
    std::vector<IData*> data;
    
    int nslots = data_port_->number_of_slots();

    uint64_t remainder;
    uint64_t nread;
    
    while (!context.terminated()) {
        
        for (int k=0; k<nslots; ++k) {
            
            if (!data_port_->slot(k)->RetrieveDataAll( data )) {break;}
            
            nread = data_port_->slot(k)->status_read();
            
            if (nread==0) { data_port_->slot(k)->ReleaseData(); continue; }
            
            if (!throttle_) {
                
                LOG_IF(WARNING,(nread>0.5*upstream_buffer_size_[k])) << name() <<
                    ": buffer is more than half full (stream " << k << ")";
                for (auto & it : data) {                
                    serializer_->Serialize( *(streams_[k]), it, k, packetid_[k]++ );
                }
                
            } else {
                
                // update throttle level
                throttle_level_ *= (1-throttle_smooth_);
                if (nread>throttle_threshold_*upstream_buffer_size_[k]) {
                    throttle_level_ += throttle_smooth_;
                }
                
                remainder = std::floor( 1.0 / ( 0.5 - std::abs(throttle_level_ - 0.5)));
                
                if (throttle_level_==0 || (throttle_level_<0.5 && remainder>nread) ) {
                    // keep all
                    for (auto & it : data) {                
                        serializer_->Serialize( *(streams_[k]), it, k, packetid_[k]++ );
                    }
                } else if (throttle_level_<0.5) {
                    // skip small fraction
                    for ( uint64_t n=0; n<nread; ++n) {
                        if (n%remainder==0) {packetid_[k]++; nskipped_[k]++; continue;}
                        serializer_->Serialize( *(streams_[k]), data[n], k, packetid_[k]++ );
                    }
                } else if (throttle_level_==1 || (throttle_level_>=0.5 && remainder>nread)) {
                    //skip all
                    packetid_[k]+=nread;
                    nskipped_[k]+=nread;
                } else {
                    //keep small fraction
                    for ( uint64_t n=0; n<nread; ++n) {
                        if (n%remainder!=0) {packetid_[k]++; nskipped_[k]++; continue;}
                        serializer_->Serialize( *(streams_[k]), data[n], k, packetid_[k]++ );
                    }
                }
            }
            
            data_port_->slot(k)->ReleaseData();
            
        }
        
    }
    
}

void FileSerializer::Postprocess( ProcessingContext& context ) {
    
    streams_.clear(); // forces destruction and closing of resources
    serializer_.reset(); 
    
    for (SlotType k=0; k<data_port_->number_of_slots(); k++) {
        if ( nskipped_[k] != 0) {
            LOG(UPDATE) << name() << ": stream " << k << ": received " << packetid_[k]
                << " data packets, of which " << nskipped_[k] << " were not saved.";
        } else {
            LOG(UPDATE) << name() << ": stream " << k << ": received and saved "
                    << packetid_[k] << " data packets.";
        }
    }
}
