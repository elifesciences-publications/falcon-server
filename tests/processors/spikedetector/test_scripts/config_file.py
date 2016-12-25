# -*- coding: utf-8 -*-
"""
Created on Wed Sep  9 20:35:02 2015

@author: davide
"""

# test_data_file = '../test_data/artificial_spikes_1s_1TT.npy'
# test_data_file = '../test_data/artificial_spikes_inverted_1s_1TT.npy'
test_data_file = '../test_data/real_spikes_1.4s_1TT.npy'
filename_mcd = '/home/davide/Data/Falcon_Outputs/spike_detection/_last_run_group/20150909_182314/sink/sink.0_source.tt1.0.bin'
filename_spikes = '/home/davide/Data/Falcon_Outputs/spike_detection/_last_run_group/_last_run/sink1/sink1.0_spikedetector1.spikes.0.bin'

sample_rate = 32000

# initial hardware timestamp of the datastream, use a negative number if you
# you will use data saved in FULL format, as in this case it will read from the
# saved data 
initial_hw_ts = -2000

# plotting parameters
min_y = -100
max_y = 150
plot_inverted = True