# -*- coding: utf-8 -*-
"""
Created on Fri Sep 11 18:06:55 2015

@author: davide
"""

from __future__ import division
import numpy as np
from matplotlib import pyplot as plt
from falcon.io.deserialization import load_spike_data
import config_file as conf


test_data = np.load(conf.test_data_file)

spike_data, tot_n_spikes, n_channels_sp, spike_orig_data, spike_header = \
    load_spike_data( conf.filename_spikes )
    
if ( conf.initial_hw_ts < 0 ):
    assert(spike_header['format'] == 'FULL')
    start_time = spike_orig_data['hardware_ts'][0] / 1e6
else:
    start_time = conf.initial_hw_ts / 1e6
    
ts_spikes = spike_orig_data['TS_detected_spikes']    
    
n_channels, n_samples = test_data.shape

time = np.linspace(0, n_samples/conf.sample_rate, n_samples)

if conf.plot_inverted:
    test_data = -test_data

f, ax = plt.subplots(4, sharex=True, sharey=True)
plt.ylim([conf.min_y, conf.max_y])
plt.yticks(np.arange(conf.min_y, conf.max_y, 25))
plt.xlabel("time [s]")
plt.gcf().set_size_inches(20, 10) # this enlargemenet is useful when visualizing data in spyder
for c in range(n_channels):
    ax[c].plot(time, test_data[c, :], color='blue')
    ax[c].plot((spike_data['times']-start_time), spike_data['amplitudes'][:, c], '*',\
        markersize=8, markerfacecolor='orange', color='red')
    ax[c].grid(which='major', linestyle='-')
    ax[c].set_title('channel {0}'.format(c))
    ax[c].set_ylabel("voltage [uV]")

    
plt.show()