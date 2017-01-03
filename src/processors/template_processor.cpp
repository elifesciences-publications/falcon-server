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

#include "template_processor.hpp"
#include "g3log/src/g2log.hpp"

constexpr datatype FalconProcessor::DEFAULT_ARGUMENT3;
constexpr datatype FalconProcessor::DEFAULT_ARGUMENT4;

void FalconProcessor::Configure(const YAML::Node& node, const GlobalContext& context) {
    
    option1_ = node["option1"].as<datatype1>( DEFAULT_OPTION1 );
    option2_ = node["option2"].as<datatype2>( );
}

void FalconProcessor::CreatePorts() {
    
    data_in_port_ = create_input_port(
        "data",
        AnyDataType(),
        PortInPolicy( SlotRange(m, n) ) );

    other_data_in_port_ = create_input_port(
        "other_data",
        AnyDataType(),
        PortInPolicy( SlotRange(m, n) ) );
    
    data_out_port_ = create_output_port(
        "data",
        AnyDataType(),
        PortOutPolicy( SlotRange(m, n) ) );

    other_data_out_port_ = create_output_port(
        "other_data",
        AnyDataType(),
        PortOutPolicy( SlotRange(m, n) ) );

    state_variable1_ = create_writable_shared_state( "state_variable1", default_state_variable1_, Permission::, Permission::);
    
    state_variable2_ = create_readable_shared_state( "state_variable2", default_state_variable2_, Permission::, Permission::);
}

void FalconProcessor::CompleteStreamInfo( ) {
    
    //
}

void FalconProcessor::Prepare( GlobalContext& context ) {

    //
}

void FalconProcessor::Preprocess( ProcessingContext& context ) {
    
    //
}

void FalconProcessor::Process(ProcessingContext& context) {
    
    AnyData* data_in = nullptr;
    AnyData* data_out = nullptr;
    T1 temp1 = 0;    

    while ( !context.terminated() ) {
        
        if (!data_in_port_->slot(0)->RetrieveData( data_in )) {break;}
        
        // place this carefully!
        data_in_port_->slot(0)->ReleaseData();
        
        // clearing will take an extra operation, don't clear if you are going to overwrite
        data_out = data_out_port_->slot(0)->ClaimData( true );
        data_out->set_hardware_timestamp( data_in->hardware_timestamp() );
        data_out->set_source_timestamp();
        data_out_port_->slot(0)->PublishData();
    }  
}

void FalconProcessor::Postprocess( ProcessingContext& context ) {
    
    LOG(INFO) << name()<< ". STREAMED " << data_port_->slot(0)->nitems_produced()
        << " data packets";
}

void FalconProcessor::Unprepare( GlobalContext& context ) {

    //
}

void FalconProcessor::method1() {
    
    //
}

