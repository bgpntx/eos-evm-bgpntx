#include "silk_engine_plugin.hpp"

#include <iostream>
#include <string>

#include <boost/process/environment.hpp>

#include <silkworm/common/log.hpp>
#include <silkworm/common/settings.hpp>
#include <silkworm/common/stopwatch.hpp>
#include <silkworm/concurrency/signal_handler.hpp>
#include <silkworm/db/stages.hpp>
#include <silkworm/stagedsync/sync_loop.hpp>
#include <silkworm/rpc/server/backend_kv_server.hpp>
#include <silkworm/rpc/util.hpp>

class silk_engine_plugin_impl : std::enable_shared_from_this<silk_engine_plugin_impl> {
   public:
      silk_engine_plugin_impl(uint32_t id, const std::string& data_dir, uint32_t num_of_threads, uint32_t max_readers, std::string address) {
         silkworm::ChainConfig config{
            id,  // chain_id
            silkworm::SealEngineType::kNoProof,
            {
               0,          // Homestead
               0,          // Tangerine Whistle
               0,          // Spurious Dragon
               0,          // Byzantium
               0,          // Constantinople
               0,          // Petersburg
               0,          // Istanbul
               // 0,          // Berlin
               // 0,          // London
            },
         };

         node_settings.data_directory = std::make_unique<silkworm::DataDirectory>(data_dir, false);
         node_settings.network_id = id;
         node_settings.etherbase  = silkworm::to_evmc_address(silkworm::from_hex("").value()); // TODO determine etherbase name
         node_settings.chaindata_env_config = {node_settings.data_directory->chaindata().path().string(), false, false};
         node_settings.chaindata_env_config.max_readers = max_readers;

         server_settings.set_address_uri(address);
         server_settings.set_num_contexts(num_of_threads);

         pid = boost::this_process::get_id();
         tid = std::this_thread::get_id();

         db_env = silkworm::db::open_env(node_settings.chaindata_env_config);
         SILK_INFO << "Created DB environment at location : " << node_settings.data_directory->chaindata().path().string();

         eth.reset(new silkworm::EthereumBackEnd(node_settings, &db_env));
         eth->set_node_name("Trust Node");
         SILK_INFO << "Created Ethereum Backend with network id <" << node_settings.network_id << ">, etherbase <" << node_settings.etherbase->bytes << ">";

         server.reset(new silkworm::rpc::BackEndKvServer(server_settings, *eth.get()));
      }

      inline void startup() {
         server->build_and_start();
         SILK_INFO << "Started Engine Server";
         boost::asio::io_context& sched = server->next_io_context();
         boost::asio::signal_set sigs{sched, SIGINT, SIGTERM};

         SILK_DEBUG << "Signals registered on scheduler " << &sched;

         sigs.async_wait([&](const boost::system::error_code& ec, int sig) {
            shutdown();
         });
         server->join();
      }

      inline void shutdown() {
         eth->close();
         server->shutdown();
         SILK_INFO << "Stopped Engine Server";
      }

      silkworm::NodeSettings                          node_settings;
      silkworm::rpc::ServerConfig                     server_settings;
      mdbx::env_managed                               db_env;
      std::unique_ptr<silkworm::EthereumBackEnd>      eth;
      std::unique_ptr<silkworm::rpc::BackEndKvServer> server;
      int                                             pid;
      std::thread::id                                 tid;
};

silk_engine_plugin::silk_engine_plugin() {}
silk_engine_plugin::~silk_engine_plugin() {}

void silk_engine_plugin::set_program_options( appbase::options_description& cli, appbase::options_description& cfg ) {
   cfg.add_options()
      ("chain-data", boost::program_options::value<std::string>()->default_value("."),
        "chain data path as a string")
      ("engine-port", boost::program_options::value<std::string>()->default_value("127.0.0.1:8080"),
        "engine address of the form <address>:<port>")
      ("contexts", boost::program_options::value<std::uint32_t>()->default_value(std::thread::hardware_concurrency() / 3),
        "number of I/O contexts")
      ("threads", boost::program_options::value<std::uint32_t>()->default_value(16),
        "number of worker threads")
   ;
}

void silk_engine_plugin::plugin_initialize( const appbase::variables_map& options ) {
   const auto& chain_data = options.at("chain-data").as<std::string>();
   const auto& address    = options.at("engine-port").as<std::string>();
   const auto& contexts   = options.at("contexts").as<uint32_t>();
   const auto& threads    = options.at("threads").as<uint32_t>();
   uint32_t id = 0; // get id from action/DB entry
   my.reset(new silk_engine_plugin_impl(id, chain_data, contexts, threads, address));
   SILK_INFO << "Initializing Silk Engine Plugin";
}

void silk_engine_plugin::plugin_startup() {
   my->startup();
}

void silk_engine_plugin::plugin_shutdown() {
   my->shutdown();
}