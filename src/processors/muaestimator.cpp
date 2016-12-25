#include "muaestimator.hpp"

#include "utilities/general.hpp"

void MUAEstimator::Configure( const YAML::Node  & node, const GlobalContext& context) {

    initial_bin_size_ = node["bin_size_ms"].as<decltype(initial_bin_size_)>( DEFAULT_BIN_SIZE );
}

void MUAEstimator::CreatePorts() {
    
    data_in_port_ = create_input_port(
        "spikes",
        SpikeDataType( ),
        PortInPolicy( SlotRange(1, 64) ) );
    
    data_out_port_ = create_output_port(
        "mua",
        MUADataType( ),
        PortOutPolicy( SlotRange(1) ) );
    
    bin_size_ = create_readable_shared_state(
        "bin_size_ms",
        initial_bin_size_,
        Permission::READ,
        Permission::WRITE );
    
    mua_ = create_writable_shared_state(
        "MUA",
        0.0,
        Permission::READ,
        Permission::READ);
}

void MUAEstimator::CompleteStreamInfo() {
    
    data_out_port_->slot(0)->datatype().Finalize( initial_bin_size_ );
    data_out_port_->streaminfo(0).Finalize( 1e3 / initial_bin_size_ );
}

void MUAEstimator::Prepare( GlobalContext& context ) {
    
    // check that all incoming SpikeData have the same buffer size
    spike_buffer_size_ = data_in_port_->streaminfo(0).datatype().buffer_size();
    if (data_in_port_->number_of_slots() > 1) {
        for ( SlotType s=1; s < data_in_port_->number_of_slots(); ++s ) {
            if (spike_buffer_size_ != data_in_port_->streaminfo(s).datatype().buffer_size()) {
                throw ProcessingConfigureError(
                    "Incoming SpikeData buffer-sizes are different.", name());
            }
        }
    }
    
    try {
        check_buffer_sizes_and_log( spike_buffer_size_, initial_bin_size_, true,
            n_spike_buffers_, name() );
    } catch( std::runtime_error& error ) {
        throw ProcessingStreamInfoError( error.what(), name() );
    }
    LOG(INFO) << name() << ". MUA will be computed using " << n_spike_buffers_
        << " spike buffers.";
    
    // TODO? if the user doesn't specify a bin_size, use the one from spikedata stream 
}

void MUAEstimator::Process( ProcessingContext& context ) {

    bool alive = true;
    
    SpikeData* data_in = nullptr;
    MUAData* data_out = nullptr;
    
    uint64_t hardware_timestamp = std::numeric_limits<uint64_t>::max();
    
    std::uint64_t spike_counter;
    
    while (!context.terminated()) {

        spike_counter = 0;
        
        current_bin_size_ = bin_size_->get();
        if ( current_bin_size_ != previous_bin_size_ ) {
            try {
                check_buffer_sizes_and_log( spike_buffer_size_, current_bin_size_,
                    true, n_spike_buffers_, name() );
            } catch( std::runtime_error& error ) {
                LOG(ERROR) << name() << ". Invalid buffer size (" << error.what() << ").";
                current_bin_size_ = previous_bin_size_;
                // recompute n_spike_buffers
                check_buffer_sizes_and_log( spike_buffer_size_, current_bin_size_,
                    true, n_spike_buffers_, name() );
            }
        }

        if ( current_bin_size_ != previous_bin_size_ ) {
            previous_bin_size_ = current_bin_size_;
            LOG(UPDATE) << ". MUA bin updated to " << current_bin_size_ << " ms.";
        }
        
        for ( decltype(n_spike_buffers_) n=0; n < n_spike_buffers_; ++n ) {
            
            for ( SlotType s=0; s < data_in_port_->number_of_slots(); ++s ) {

                alive = data_in_port_->slot(s)->RetrieveData( data_in ); 

                if (not alive) {break;}

                if (s==0) {
                    hardware_timestamp = data_in->hardware_timestamp();
                } else if (data_in->hardware_timestamp() != hardware_timestamp ) {
                        throw ProcessingError( "Synchronization error", name() );
                }
                
                spike_counter += data_in->n_detected_spikes();
                data_in_port_->slot(s)->ReleaseData();
            }
        }
        
        data_out = data_out_port_->slot(0)->ClaimData( false );
        data_out->set_bin_size( current_bin_size_ );
        data_out->set_n_spikes( spike_counter );
        data_out->set_hardware_timestamp( hardware_timestamp );
        
        mua_->set( data_out->mua() );
        
        data_out_port_->slot(0)->PublishData(); 
        
    }
}
