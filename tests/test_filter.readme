Examples for test_filter
------------------------

Get help:

>> test_filter -?

Just import filter and show description:

>> test_filter filterfile

Time filter on 1000000 point long signal with 2 channels:

>> test_filter -t -n 1000000 -c 2 filterfile

Filter custom signal and save result in result.dat:

>> test_filter -s noise.dat -o result.dat filterfile

Compute impulse response and save to imp.dat.
By default the impulse response has as many points as the order of the 
filter. For IIR filters you want to set the number of points yourself
 using the -p flag.

>> test_filter -i -f imp.dat filterfile

