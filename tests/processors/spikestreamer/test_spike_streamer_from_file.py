# -*- coding: utf-8 -*-
"""
Created on Thu Sep 10 18:18:43 2015

@author: davide
"""

# ---------------------------------------------------------------------
# This file is part of falcon-server.
#
# Copyright (C) 2015, 2016, 2017 Neuro-Electronics Research Flanders
#
# Falcon-server is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Falcon-server is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with falcon-server. If not, see <http://www.gnu.org/licenses/>.
# ---------------------------------------------------------------------


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
    
