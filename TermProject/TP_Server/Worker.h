#pragma once
#include "headers.h"

class Session;

void WorkerThread();

extern HANDLE g_h_iocp;
extern tbb::concurrent_unordered_map<uint64_t, std::shared_ptr<Session>> g_clients;
