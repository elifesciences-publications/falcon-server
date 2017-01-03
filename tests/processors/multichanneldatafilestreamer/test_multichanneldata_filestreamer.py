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
from falcon.io.deserialization import load_file
import config_file as config

data, header = load_file(config.filename)
assert(header['format'] == 'COMPACT')
generated_data = np.load("random_data.npy") 

n_samples = len(data)
n_channels = int(header['data'][1][-2])

# check timestamps
timestamps = data['timestamps']
dt = np.diff( timestamps )
period_us = 1e6 / config.sample_rate
dt_lower_bound = np.trunc(period_us)
dt_upper_bound = dt_lower_bound + 1
 
if np.all(generated_data == data['signal'].T) and \
(config.initial_timestamp == timestamps[0]) and \
np.all(np.logical_or(dt == dt_lower_bound, dt == dt_upper_bound)):
    print "test passed"
else:
    print "test not passed"
