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

#ifndef THREADUTILITIES_H
#define THREADUTILITIES_H

#include <thread>

typedef int16_t ThreadPriority;

const ThreadPriority PRIORITY_NONE = -1;
const ThreadPriority PRIORITY_MIN = 0;
const ThreadPriority PRIORITY_MEDIUM = 50;
const ThreadPriority PRIORITY_HIGH = 75;
const ThreadPriority PRIORITY_MAX = 100;

typedef int16_t ThreadCore;

const ThreadCore CORE_NOT_PINNED = -1;

bool set_realtime_priority( pthread_t thread, ThreadPriority priority = PRIORITY_NONE );

bool set_thread_core( pthread_t thread, ThreadCore core );

#endif
