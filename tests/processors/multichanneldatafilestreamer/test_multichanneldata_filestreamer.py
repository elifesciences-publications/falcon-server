# -*- coding: utf-8 -*-
"""
Created on Thu Sep 10 18:18:43 2015

@author: davide
"""

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