/* DigitalOutput: takes an EventData stream and sets digital outputs
 * according to an event-specific protocol
 * 
 * input ports:
 * events <EventData> (1 slot)
 *
 * output ports:
 * none
 *
 * exposed states:
 * enabled <bool> - enable/disable digital output
 *
 * exposed methods:
 * none
 *
 * options:
 * enabled <bool> - default for enabled state
 * pulse_width <unsigned int> - duration of digital output pulse in microseconds
 * device - map specifying the digital output device. A required "type" 
 *   key indicates which device should be used. Valid values are "dummy"
 *   and "advantech". The dummy device requires an additional "nchannels"
 *   key. The advantech device needs an additional "description" key,
 *   and has optional port and delay keys. If port<0, then all ports on
 *   the device are used, otherwise the specified port will be used.
 *   The delay specifies, in microseconds, the imposed delay before
 *   reading or writing a state. If all port<0, then higher port numbers
 *   will have higher delays, because individual p1orts on the Advantech
 *   device are read/written separately in sequence. 
 * protocols - maps events to digital output protocols (see below)
 * 
 * extra information:
 * The protocols option specifies a map with  for each target event
 * a map of actions for selected digital output channels. Note that each
 * channel can only be associated with a single action (even if it is
 * listed more than once). There are 4 possible actions: high, low, 
 * toggle and pulse. Events that are not in the protocols map are
 * ignored. Example configuration for protocols option:
 * 
 * protocols:
 *   event_a:
 *     high: [0,1]
 *   event_b:
 *     low: [0]
 *     toggle: [1]
 *   event_c:
 *     pulse: [2]
 * 
 */

#ifndef DIGITALOUTPUT_HPP
#define DIGITALOUTPUT_HPP

#include "../graph/iprocessor.hpp"
#include "../data/eventdata.hpp"
#include "dio/dio.hpp"
#include "utilities/time.hpp"

typedef std::map<std::string,std::map<std::string,std::vector<uint32_t>>> ProtocolYAMLMap;
typedef std::map<std::string,std::unique_ptr<DigitalOutputProtocol>> ProtocolMap;

class DigitalOutput : public IProcessor {
    
public:
    virtual void Configure(const YAML::Node& node, const GlobalContext& context) override;
    virtual void CreatePorts() override;
    virtual void Preprocess( ProcessingContext& context ) override;
    virtual void Process( ProcessingContext& context ) override;
    virtual void Postprocess( ProcessingContext& context ) override;
    

protected:
    bool to_lock_out( const uint64_t current_timestamp );
    
protected:
    PortIn<EventDataType>* data_in_port_;
    
    bool default_enabled_;
    ReadableState<decltype(default_enabled_)>* enabled_state_;
    
    int default_lockout_period_ms_;
    ReadableState<decltype(default_lockout_period_ms_)>* lockout_period_ms_; 
    
    bool save_stim_events_;
    std::wstring device_name_;
    
    std::unique_ptr<DigitalDevice> device_;
    ProtocolMap protocols_;
    
    bool print_protocol_execution_updates_;
    
    std::uint64_t nreceived_events_;
    decltype(nreceived_events_) ntarget_events_;
    decltype(nreceived_events_) nprotocol_executions_;
    decltype(nreceived_events_) n_locked_out_events_;
    
    uint64_t previous_TS_nostim_;
    decltype(default_lockout_period_ms_) delta_TS_ms_;
    
public:
    const decltype(default_enabled_) DEFAULT_ENABLED = true;
    const decltype(default_lockout_period_ms_) DEFAULT_LOCKOUT_PERIOD_MS = 300;
    const decltype(save_stim_events_) DEFAULT_SAVE_STIM_EVENTS = true;
    const unsigned int DEFAULT_PULSE_WIDTH_MICROSEC = 400;
    const std::string DEFAULT_ADVANTECH_DEVICE = "USB-4750, BID#0";
    const unsigned int DEFAULT_DUMMY_NCHANNELS = 16;
    const int DEFAULT_ADVANTECH_PORT = -1;
    const std::uint64_t DEFAULT_ADVANTECH_DELAY = 10;
    
protected:
    const std::string STIM_EVENT_S = "stim_";
};

#endif // digitaloutput.hpp
