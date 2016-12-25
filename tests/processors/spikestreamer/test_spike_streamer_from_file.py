# -*- coding: utf-8 -*-
"""
Created on Thu Sep 10 18:18:43 2015

@author: davide
"""

import numpy as np
from falcon.io.deserialization import load_spike_data
import config_file as config

spike_data, tot_n_spikes, n_channels, data, header = load_spike_data(config.filename_falcon_output)
spike_amplitudes = np.load(config.filename_spike_amplitudes) 
spike_times = np.load(config.filename_spike_times)

assert (n_channels == config.n_channels)

times_match = np.max( np.abs(spike_data['times'] - spike_times)) < config.error_float

amplitudes_match = np.max( np.abs(spike_data['amplitudes'].flatten() - spike_amplitudes)) < config.error_float

if not times_match:
    print "Times do NOT match"

if not amplitudes_match:
    print "Spike amplitudes do NOT match"    
    
if times_match and amplitudes_match:
    print "\nTest PASSED"
else:
    print "\nTest NOT passed"    
    