#include "multichannelfilter.hpp"

#include "g3log/src/g2log.hpp"
#include <thread>
#include <chrono>
#include <exception>

void MultiChannelFilter::CreatePorts( ) {
    
    data_in_port_ = create_input_port(
        "data",
        MultiChannelDataType<double>( ChannelRange(1,256) ),
        PortInPolicy( SlotRange(0,256) ) );
    
    data_out_port_ = create_output_port(
        "data",
        MultiChannelDataType<double>( ChannelRange(1,256) ),
        PortOutPolicy( SlotRange(0,256) ) );
}

void MultiChannelFilter::Configure( const YAML::Node & node, const GlobalContext& context ) {
    
    // load filter
    if (!node["filter"]) {
        throw ProcessingConfigureError( "No filter defined.", name() );
    }
    
    if (node["filter"]["file"]) {
        std::string f = context.resolve_path( node["filter"]["file"].as<std::string>(),
            "filters" );
        filter_template_.reset( dsp::filter::construct_from_file( f ) );
    } else {
        filter_template_.reset( dsp::filter::construct_from_yaml( node["filter"] ) );
    }
}

void MultiChannelFilter::CompleteStreamInfo( ) {
    
    // check if we have the same number of input and output slots
    if (data_in_port_->number_of_slots() != data_out_port_->number_of_slots()) {
        auto err_msg = "Number of output slots (" +
            std::to_string( data_out_port_->number_of_slots() ) + 
            ") on port '" + data_out_port_->name() +
            "' does not match number of input slots (" +
            std::to_string( data_in_port_->number_of_slots()) +
            ") on port '" + data_in_port_->name() + "'.";
        throw ProcessingStreamInfoError( err_msg, name() );
    }
    
    for ( int k=0; k<data_in_port_->number_of_slots(); ++k ) {
        data_out_port_->streaminfo(k).datatype().Finalize(
            data_in_port_->streaminfo(k).datatype() );
        data_out_port_->streaminfo(k).Finalize(
            data_in_port_->streaminfo(k).stream_rate() );
    }
    
}

void MultiChannelFilter::Prepare( GlobalContext& context ) {
    
    // realize filter for each input slot, dependent on the number of channels upstream is sending
    filters_.clear();
    for (int k=0; k<data_in_port_->number_of_slots(); ++k ) {
        filters_.push_back( std::move(
            std::unique_ptr<dsp::filter::IFilter>( filter_template_->clone() ) ) );
        filters_.back()->realize( data_in_port_->streaminfo(k).datatype().nchannels() );
    }
}

void MultiChannelFilter::Unprepare( GlobalContext& context ) {
    
    // destroy realized filters
    filters_.clear();
}

void MultiChannelFilter::Preprocess( ProcessingContext& context ) {}

void MultiChannelFilter::Process( ProcessingContext& context ) {
    
    
    MultiChannelData<double>* data_in = nullptr;
    MultiChannelData<double>* data_out = nullptr;
    
    auto nslots = data_in_port_->number_of_slots();
    decltype(nslots) k=0;
    
    while (!context.terminated()) {
        
        // go through all slots
        for (k=0; k<nslots; ++k) {
            
            // retrieve new data
            if (!data_in_port_->slot(k)->RetrieveData( data_in )) {break;}
            
            // claim output data buckets
            data_out = data_out_port_->slot(k)->ClaimData(false);
            
            // filter incoming data               
            filters_[k]->process_by_channel(
                data_in->nsamples(), data_in->data(), data_out->data() );
            
            data_out->set_sample_timestamps( data_in->sample_timestamps() );
            
            data_out->CloneTimestamps( *data_in );
            
            // publish and release data
            data_out_port_->slot(k)->PublishData();
            data_in_port_->slot(k)->ReleaseData();
            
        }
                
    }
}

void MultiChannelFilter::Postprocess( ProcessingContext& context ) {}
