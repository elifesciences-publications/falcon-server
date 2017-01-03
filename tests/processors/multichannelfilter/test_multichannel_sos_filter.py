# -*- coding: utf-8 -*-
"""
Created on Wed Sep  9 17:51:01 2015

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


from __future__ import division
import numpy as np
from matplotlib import pyplot as plt
from falcon.io.deserialization import load_file
from scipy.signal import lfilter
import config_file as conf


data_in, header_in = load_file( conf.filename_input_signal )
data_out, header_out = load_file( conf.filename_output_signal )

n_channels_in = int(header_in['data'][1][-2])
n_channels_out = int(header_out['data'][1][-2])
assert ( n_channels_in == n_channels_out )

start_time = data_out['timestamps'][0]/1e6
assert ( start_time == data_in['timestamps'][0]/1e6 )

b, a = np.load( conf.ba_coefficients_filename )
offline = []
for c in range(n_channels_in):    
    offline.append( lfilter(b, a, data_in['signal'][:, c]) )

t = data_out['timestamps'][:]/1e6 - start_time

# plot both signals together
f1, ax1 = plt.subplots(n_channels_in, sharex=True, sharey=True)
plt.ylim([conf.min_y, conf.max_y])
plt.xlim([0, data_out['timestamps'][-1]/1e6 - start_time])
plt.xlabel("time [s]")
plt.gcf().set_size_inches(20, 8)
fig1 = plt.figure(1)
for c in range(n_channels_in):
    ax1[c].plot(t, data_out['signal'][:, c], color='blue')
    ax1[c].plot(t, offline[c], color='green')
    ax1[c].grid(which='major', linestyle='-')
    ax1[c].set_title('channel {0}'.format(c))
    ax1[c].set_ylabel("voltage [uV]")
    
# compute and plot error between the two signals
error = np.abs(offline - data_out['signal'].T)
f2, ax2 = plt.subplots(n_channels_in, sharex=True, sharey=True)
plt.xlim([0, data_out['timestamps'][-1]/1e6 - start_time])
plt.xlabel("time [s]")
plt.gcf().set_size_inches(20, 8)
fig2 = plt.figure(2)
for c in range(n_channels_in):
    ax2[c].plot(t, np.log10(error[c, :]), color='red')
    ax2[c].set_title('channel {0}'.format(c))
    ax2[c].set_ylabel("difference [log10(uV])")
    
plt.show()
