#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "disruptor/interface.h"
#include "disruptor/ring_buffer.h"
#include "disruptor/claim_strategy.h"
#include "disruptor/wait_strategy.h"
#include "disruptor/batch_descriptor.h"

template <typename DATA>
using RingBuffer = disruptor::RingBuffer<DATA>;

template <typename DATA>
using IFactory = disruptor::EventFactoryInterface<DATA>;

typedef disruptor::ProcessingSequenceBarrier RingBarrier;

typedef disruptor::Sequence RingSequence;

typedef disruptor::ClaimStrategyOption ClaimStrategy;
typedef disruptor::WaitStrategyOption WaitStrategy;
typedef disruptor::AlertException RingAlertException;

typedef disruptor::BatchDescriptor RingBatch;
#endif
