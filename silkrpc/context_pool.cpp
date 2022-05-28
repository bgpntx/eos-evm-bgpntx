/*
   Copyright 2020 The Silkrpc Authors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "context_pool.hpp"

#include <stdexcept>
#include <thread>
#include <utility>

#include <silkrpc/common/log.hpp>
#include <silkrpc/ethdb/kv/remote_database.hpp>
#include <silkrpc/ethbackend/remote_backend.hpp>
#include <silkworm/common/assert.hpp>

namespace silkrpc {

std::ostream& operator<<(std::ostream& out, Context& c) {
    out << "io_context: " << c.io_context() << " queue: " << c.grpc_queue() << " end_point: " << c.rpc_end_point();
    return out;
}

bool AbslParseFlag(absl::string_view text, WaitMode* wait_mode, std::string* error) {
    if (text == "blocking") {
        *wait_mode = WaitMode::blocking;
        return true;
    }
    if (text == "sleeping") {
        *wait_mode = WaitMode::sleeping;
        return true;
    }
    if (text == "yielding") {
        *wait_mode = WaitMode::yielding;
        return true;
    }
    if (text == "spin_wait") {
        *wait_mode = WaitMode::spin_wait;
        return true;
    }
    if (text == "busy_spin") {
        *wait_mode = WaitMode::busy_spin;
        return true;
    }
    *error = "unknown value for WaitMode";
    return false;
}

std::string AbslUnparseFlag(WaitMode wait_mode) {
    switch (wait_mode) {
        case WaitMode::blocking: return "blocking";
        case WaitMode::sleeping: return "sleeping";
        case WaitMode::yielding: return "yielding";
        case WaitMode::spin_wait: return "spin_wait";
        case WaitMode::busy_spin: return "busy_spin";
        default: return absl::StrCat(wait_mode);
    }
}

std::unique_ptr<WaitStrategy> make_wait_strategy(WaitMode wait_mode) {
    switch (wait_mode) {
        case WaitMode::yielding:
            return std::make_unique<YieldingWaitStrategy>();
        case WaitMode::sleeping:
            return std::make_unique<SleepingWaitStrategy>();
        case WaitMode::spin_wait:
            return std::make_unique<SpinWaitWaitStrategy>();
        case WaitMode::busy_spin:
            return std::make_unique<BusySpinWaitStrategy>();
        default:
            return nullptr;
    }
}

Context::Context(ChannelFactory create_channel, std::shared_ptr<BlockCache> block_cache, WaitMode wait_mode)
    : io_context_{std::make_shared<asio::io_context>()},
      work_{asio::require(io_context_->get_executor(), asio::execution::outstanding_work.tracked)},
      queue_{std::make_unique<grpc::CompletionQueue>()},
      block_cache_(block_cache),
      wait_mode_(wait_mode) {
    std::shared_ptr<grpc::Channel> channel = create_channel();
    rpc_end_point_ = std::make_unique<silkworm::rpc::CompletionEndPoint>(*queue_);
    database_ = std::make_unique<ethdb::kv::RemoteDatabase<>>(*io_context_, channel, queue_.get());
    backend_ = std::make_unique<ethbackend::RemoteBackEnd>(*io_context_, channel, queue_.get());
    miner_ = std::make_unique<txpool::Miner>(*io_context_, channel, queue_.get());
    tx_pool_ = std::make_unique<txpool::TransactionPool>(*io_context_, channel, queue_.get());
}

void Context::execute_loop() {
    if (wait_mode_ == WaitMode::blocking) {
        execute_loop_double_threaded();
    } else {
        execute_loop_single_threaded();
    }
}

void Context::execute_loop_single_threaded() {
    SILKRPC_INFO << "Single-thread execution loop start [" << this << "]\n";
    std::unique_ptr<WaitStrategy> wait_strategy{make_wait_strategy(wait_mode_)};
    SILKWORM_ASSERT(wait_strategy != nullptr);
    while (!io_context_->stopped()) {
        uint32_t executed_count = rpc_end_point_->poll_one();
        executed_count += io_context_->poll_one();
        wait_strategy->wait_once(executed_count);
    }
    rpc_end_point_->shutdown();
    SILKRPC_INFO << "Single-thread execution loop end [" << this << "]\n";
}

void Context::execute_loop_double_threaded() {
    SILKRPC_INFO << "Double-thread execution loop start [" << this << "]\n";
    std::thread completion_runner_thread{[&]() {
        bool stopped{false};
        while (!stopped) {
            stopped = rpc_end_point_->post_one(*io_context_);
        }
    }};
    io_context_->run();
    rpc_end_point_->shutdown();
    completion_runner_thread.join();
    SILKRPC_INFO << "Double-thread execution loop end [" << this << "]\n";
}

void Context::stop() {
    io_context_->stop();
}

ContextPool::ContextPool(std::size_t pool_size, ChannelFactory create_channel, WaitMode wait_mode) : next_index_{0} {
    if (pool_size == 0) {
        throw std::logic_error("ContextPool::ContextPool pool_size is 0");
    }
    SILKRPC_INFO << "ContextPool::ContextPool creating pool with size: " << pool_size << "\n";

    // Create the unique block cache to be shared among the execution contexts.
    auto block_cache = std::make_shared<silkrpc::BlockCache>();

    // Create as many execution contexts according as required by the pool size.
    for (std::size_t i{0}; i < pool_size; ++i) {
        contexts_.push_back(Context{create_channel, block_cache, wait_mode});
        SILKRPC_DEBUG << "ContextPool::ContextPool context[" << i << "] " << contexts_[i] << "\n";
    }
}

ContextPool::~ContextPool() {
    SILKRPC_TRACE << "ContextPool::~ContextPool started " << this << "\n";
    stop();
    SILKRPC_TRACE << "ContextPool::~ContextPool completed " << this << "\n";
}

void ContextPool::start() {
    SILKRPC_TRACE << "ContextPool::start started\n";

    if (!stopped_) {
        // Create a pool of threads to run all of the contexts (each one having 1 threads)
        for (std::size_t i{0}; i < contexts_.size(); ++i) {
            auto& context = contexts_[i];
            context_threads_.create_thread([&, i = i]() {
                SILKRPC_DEBUG << "thread start context[" << i << "] thread_id: " << std::this_thread::get_id() << "\n";
                context.execute_loop();
                SILKRPC_DEBUG << "thread end context[" << i << "] thread_id: " << std::this_thread::get_id() << "\n";
            });
            SILKRPC_DEBUG << "ContextPool::start context[" << i << "].io_context started: " << &*context.io_context() << "\n";
        }
    }

    SILKRPC_TRACE << "ContextPool::start completed\n";
}

void ContextPool::join() {
    SILKRPC_TRACE << "ContextPool::join started\n";

    // Wait for all threads in the pool to exit.
    SILKRPC_DEBUG << "ContextPool::join joining...\n";
    context_threads_.join();

    SILKRPC_TRACE << "ContextPool::join completed\n";
}

void ContextPool::stop() {
    // Explicitly stop all scheduler runnable components
    SILKRPC_TRACE << "ContextPool::stop started\n";

    for (std::size_t i{0}; i < contexts_.size(); ++i) {
        contexts_[i].stop();
        SILKRPC_DEBUG << "ContextPool::stop context[" << i << "].io_context stopped: " << &*contexts_[i].io_context() << "\n";
    }
    SILKRPC_TRACE << "ContextPool::stop completed\n";
}

void ContextPool::run() {
    start();
    join();
}

Context& ContextPool::next_context() {
    // Use a round-robin scheme to choose the next context to use
    auto& context = contexts_[next_index_];
    next_index_ = ++next_index_ % contexts_.size();
    return context;
}

asio::io_context& ContextPool::next_io_context() {
    auto& client_context = next_context();
    return *client_context.io_context();
}

} // namespace silkrpc
