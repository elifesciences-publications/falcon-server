// ---------------------------------------------------------------------
// This file is part of falcon-server.
// 
// Copyright (C) 2015, 2016, 2017 Neuro-Electronics Research Flanders
// 
// Falcon-server is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Falcon-server is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with falcon-server. If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------

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
