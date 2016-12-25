template <typename ForwardIterator>
void update_slope( ForwardIterator sample ) {
    for (decltype(nchannels_) c=0; c < nchannels_; ++c) {
        if (previous_sample_[c]!=*sample) { // deal with plateaus
            slope_[c] = *sample - previous_sample_[c];
        }
        ++sample;
    }
}
