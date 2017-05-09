#include "openephysreader.hpp"

#include "g3log/src/g2log.hpp"

void OpenEphysReader::Configure( const YAML::Node & node, const GlobalContext& context ) {
    
    // acquisition entities : map of vector<int>
    if (node["channelmap"]) {
        channelmap_ = node["channelmap"].as<decltype(channelmap_)>();
    }
    
    // how many packets to pack into single multi-channel data bucket
    batch_size_ = node["batch_size"].as<decltype(batch_size_)>( DEFAULT_BATCHSIZE );
    if ( batch_size_ < SAMPLES_PER_DATA_BLOCK ) {
        auto msg = "Batch size should be of at least " + std::to_string(
            SAMPLES_PER_DATA_BLOCK ) + " samples.";
        throw ProcessingConfigureError( msg, name() );
    }
    
    // number of AD channels of the system
    nchannels_ = node["nchannels"].as<decltype(nchannels_)>(DEFAULT_NCHANNELS);
    
    // how often updates about data stream will be sent out
    decltype(update_interval_) value = node["update_interval"].as<decltype(
        update_interval_)>(DEFAULT_UPDATE_INTERVAL_SEC);
    update_interval_ = value * OpenEphys::SIGNAL_SAMPLING_FREQUENCY;
    if (update_interval_==0) {
        update_interval_ = std::numeric_limits<uint64_t>::max();
    }
}

void OpenEphysReader::CreatePorts() {
    
    for (auto & it : channelmap_ ) {
        
        data_ports_[it.first] = create_output_port(
            it.first,
            MultiChannelDataType<double>( ChannelRange(it.second.size()) ),
            PortOutPolicy( SlotRange(1), 500, WaitStrategy::kBlockingStrategy ) );
    }
}

void OpenEphysReader::CompleteStreamInfo() {
    
    for (auto & it : data_ports_ ) {
        
        it.second->streaminfo(0).datatype().Finalize(
            batch_size_,
            channelmap_[it.first].size(),
            OpenEphys::SIGNAL_SAMPLING_FREQUENCY );
        it.second->streaminfo(0).Finalize(
            OpenEphys::SIGNAL_SAMPLING_FREQUENCY / batch_size_ );
    }
}

void OpenEphysReader::Prepare( GlobalContext& context ) {
    
    deviceFound = false;
    
    eval_board_.reset( new Rhd2000EvalBoard );
    if ( eval_board_->open() == 1 ) {
        deviceFound = true;
        LOG(UPDATE) << name() << ". Board opened.";
    } else {
        throw ProcessingPrepareError( "Opal Kelly board not found.", name() );
    }
    
    
    initialize_board( context.resolve_path( "repo://lib/openephys/main.bit" ) );
    LOG(UPDATE) << name() << ". Board initialized.";
    
    // Select per-channel amplifier sampling rate.
    auto sampling_rate = static_cast<Rhd2000EvalBoard::AmplifierSampleRate>(
        OpenEphys::SIGNAL_SAMPLING_FREQUENCY );
    eval_board_->setSampleRate( sampling_rate );
    LOG(INFO) << name() << ". Sample rate set to " << eval_board_->getSampleRate();

    // Now that we have set our sampling rate, we can set the MISO sampling delay
    // which is dependent on the sample rate.
    eval_board_->setCableLengthFeet( Rhd2000EvalBoard::PortA, OpenEphys::FOOT_CABLELENGTH );

    chipRegisters_ = new Rhd2000Registers( eval_board_->getSampleRate() );
    updateRegisters( chipRegisters_ );
    
    // we assume the board operates always with only portA1 and therefore only
    // one data stream is enabled
    eval_board_->setDataSource( OpenEphys::DEFAULT_DATASTREAM, Rhd2000EvalBoard::PortA1 );
    LOG(INFO) << name() << ". Board initialized. Data source set to A1.";
    eval_board_->enableDataStream( OpenEphys::DEFAULT_DATASTREAM, true);

    if (eval_board_->getNumEnabledDataStreams() != 1) {
        throw ProcessingPrepareError( ". Unexpected number of data streams enabled.", name() );
    }
}

void OpenEphysReader::Preprocess( ProcessingContext& context ) {

    sample_counter_ = 0;
}

void OpenEphysReader::Process( ProcessingContext& context ) {
      
    std::vector<MultiChannelData<double>*> data_vector(data_ports_.size());
    
    bool acquisition_finished = false;
    bool usbDataRead;
    bool first_cycle = true;
    
    while ( !context.terminated() && !acquisition_finished ) {
        
        if ( first_cycle ) {
            if ( startAcquisition() ) {
                LOG(UPDATE) << name() << ". Acquisition started successfully.";
            } else {
                LOG(ERROR) << name() << ". Acquisition failed to start.";
            }
            first_cycle = false;
        }

        usbDataRead = eval_board_->readDataBlocks( 1, data_queue_ );
        
        if ( not usbDataRead ) {
            LOG(DEBUG) << name() << ". USB data was not read."; 
        } else {
            LOG_IF( UPDATE, (sample_counter_==0) ) << name() <<
                ". First USB data block was read.";
            sample_counter_ = sample_counter_ + SAMPLES_PER_DATA_BLOCK;
            if ( sample_counter_%batch_size_==0 ) {
                write_records( data_queue_.front(), data_vector );
                data_queue_.pop();
            }
        }

        send_updates( usbDataRead );
    }
}

void OpenEphysReader::Postprocess( ProcessingContext& context ) {
    
    if ( stopAcquisition() ) {
        LOG(UPDATE) << name() << ". Acquisition stopped successfully.";
    } else {
        LOG(ERROR) << name() << ". Acquisition failed to stop.";
    }
    
    SlotType s;
    for (auto & it : data_ports_ ) {
        for (s=0; s < it.second->number_of_slots(); ++s) {
            LOG(INFO) << name()<< ". Port " << it.first << ". Slot " << s <<
                ". Streamed " << it.second->slot(s)->nitems_produced() <<
                " data packets. ";
        }
    }
}

void OpenEphysReader::Unprepare( GlobalContext& context ) {
    
    delete chipRegisters_; chipRegisters_ = nullptr;

    if (deviceFound) {
        
        int ledArray[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        eval_board_->setLedDisplay(ledArray);
    }

    if (deviceFound) {
        eval_board_->resetBoard();
        eval_board_->resetFpga();
    }
    
    LOG(INFO) << "RHD2000 interface destroyed.";
}

bool OpenEphysReader::startAcquisition() {
    
    LOG(UPDATE) << name() << ". Starting acquisition.";
    
    int ledArray[8] = {1, 1, 0, 0, 0, 0, 0, 0};
    eval_board_->setLedDisplay(ledArray);

    LOG(UPDATE) << name() << ". Number of 16-bit words in FIFO: " << eval_board_->numWordsInFifo();
    LOG(DEBUG) << name() << ". Is eval board running: " << eval_board_->isRunning();
    
//    LOG(UPDATE) << name() << ". Flushing FIFO.";
//    eval_board_->flush();
//    LOG(UPDATE) << name() << ". FIFO Flushed.";
    eval_board_->setContinuousRunMode(true);
    LOG(DEBUG) << name() << ". Continuous mode set.";
    eval_board_->run();
    LOG(DEBUG) << name() << ". Toggle run.";

    auto dataBlock = new Rhd2000DataBlock(eval_board_->getNumEnabledDataStreams());
    LOG(DEBUG) << name() << ". dataBlock created.";
    auto blockSize = dataBlock->calculateDataBlockSizeInWords(
        eval_board_->getNumEnabledDataStreams() );
    LOG(DEBUG) << name() << ". Expecting blocksize of " << blockSize << " for "
        << eval_board_->getNumEnabledDataStreams() << " streams";
    
    delete dataBlock; dataBlock = nullptr;
    LOG(DEBUG) << name() << ". dataBlock deleted.";

    return true;
}

bool OpenEphysReader::stopAcquisition() {

    LOG(UPDATE) << name() << ". Stopping acquisition.";

    if (deviceFound) {
        
        eval_board_->setContinuousRunMode(false);
        eval_board_->setMaxTimeStep(0);
        LOG(UPDATE) << name() << ". Flushing FIFO.";
        eval_board_->flush();
        
        LOG(UPDATE) << name() << "Number of 16-bit words in FIFO: " <<
            eval_board_->numWordsInFifo() << endl;

        int ledArray[8] = {1, 0, 0, 0, 0, 0, 0, 0};
        eval_board_->setLedDisplay(ledArray);
        
    } else {
        return false;
    }

    return true;
}

void OpenEphysReader::initialize_board( std::string fpgaconfig_filename ) {

    // Load Rhythm FPGA configuration bitfile (provided by Intan Technologies).
    deviceFound &= eval_board_->uploadFpgaBitfile( fpgaconfig_filename );
    eval_board_->initialize();

    // Select per-channel amplifier sampling rate.
    auto sampling_rate = static_cast<Rhd2000EvalBoard::AmplifierSampleRate>(
        OpenEphys::SIGNAL_SAMPLING_FREQUENCY );
    eval_board_->setSampleRate( sampling_rate );
    LOG(INFO) << name() << ". Sampling rate set to: " << eval_board_->getSampleRate()
            << " Hz.";

    // Now that we have set our sampling rate, we can set the MISO sampling delay
    // which is dependent on the sample rate.  We assume a 3-foot cable.
    eval_board_->setCableLengthFeet(Rhd2000EvalBoard::PortA, 3.0);
    LOG(INFO) << name() << ". Cable length set.";

    // Since our longest command sequence is 60 commands, run the SPI interface for
    // 60 samples (64 for usb3 power-of two needs)
    eval_board_->setMaxTimeStep(60);
    eval_board_->setContinuousRunMode(false);
    // Start SPI interface
    eval_board_->run();
    // Wait for the 60-sample run to complete
    while (eval_board_->isRunning()) {
        /*do nothing*/;
    }

    // Read the resulting single data block from the USB interface. We don't
    // need to do anything with this, since it was only used for ADC calibration
    // Read one data block
    bool usbDataRead;
    do {
        usbDataRead = eval_board_->readDataBlocks(1, data_queue_);
    } while (usbDataRead || eval_board_->isRunning());

    // Let's turn one LED on to indicate that the board is now connected
    int ledArray[8] = {1, 0, 0, 0, 0, 0, 0, 0};
    eval_board_->setLedDisplay(ledArray);

}

void OpenEphysReader::updateRegisters( Rhd2000Registers* chipRegisters ) {
    
    if ( chipRegisters == nullptr ) {
        LOG(ERROR) << name() << ". Invalid chip register.";
    }
    
    chipRegisters->defineSampleRate( OpenEphys::SIGNAL_SAMPLING_FREQUENCY );
    // Before generating register configuration command sequences, set amplifier
    // bandwidth paramters.
    auto actualLowerBandwidth = chipRegisters->setLowerBandwidth(OpenEphys::LOWER_BANDWIDTH);
    LOG(INFO) << name() << ". Actual lower bandwidth frequency: " << actualLowerBandwidth
        << " Hz.";
    auto actualUpperBandwidth = chipRegisters->setUpperBandwidth(OpenEphys::UPPER_BANDWIDTH);
    LOG(INFO) << name() << ". Actual upper bandwidth frequency: " << actualUpperBandwidth
        << " Hz.";
    chipRegisters->enableDsp(OpenEphys::DSP_ENABLED);
    LOG(INFO) << name() << ". DSP enabled.";
    auto actualDspCutoffFreq = chipRegisters->setDspCutoffFreq(OpenEphys::DSP_CUTOFF);
    LOG(INFO) << name() << ". Actual DSP cutoff frequency: " << actualDspCutoffFreq
        << " Hz.";
}

void OpenEphysReader::write_records( Rhd2000DataBlock origin,
    std::vector<MultiChannelData<double>*>& data_vector ) {

    // claim new data buckets and set data bucket metadata
    int port_index = 0;
    for (auto & it : data_ports_ ) {
        data_vector[port_index] = it.second->slot(0)->ClaimData(true);
        data_vector[port_index]->set_hardware_timestamp(
            origin.timeStamp[0] );
        data_vector[port_index]->set_source_timestamp();
        ++ port_index;
    }
    
    // actual write
    size_t ch;
    for (unsigned int t=0; t < batch_size_; ++t) {

        auto it = channelmap_.begin();
        for ( unsigned int pi=0; pi<channelmap_.size(); pi++ ) {

            data_vector[pi]->set_sample_timestamp(
                t,
                origin.timeStamp[t]);
            ch = 0;
            for (auto& channel : it->second) {
                data_vector[pi]->set_data_sample(
                    t,
                    ch,
                    OpenEphys::ADbits_to_microvolts( origin.amplifierData[
                        OpenEphys::DEFAULT_DATASTREAM][channel][t]));
                ++ ch;
            }
            ++ it;
        }
    }
    
    // publish data buckets
    for (auto & ite : data_ports_ ) {
        ite.second->slot(0)->PublishData();
    }
}

void OpenEphysReader::send_updates( bool usbDataRead ) { 
    
    bool update_time = (sample_counter_%update_interval_== 0 and
        sample_counter_>0 and usbDataRead);
    LOG_IF(UPDATE, update_time ) << name() << ": " <<
        sample_counter_ << " packets (" <<
        sample_counter_/OpenEphys::SIGNAL_SAMPLING_FREQUENCY <<
            " s) received.";
}
