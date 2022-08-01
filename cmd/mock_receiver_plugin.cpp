#include "mock_receiver_plugin.hpp"

#include <iostream>
#include <string>


#include <boost/circular_buffer.hpp>

#include <silkworm/consensus/engine.hpp>
#include <silkworm/execution/analysis_cache.hpp>
#include <silkworm/execution/evm.hpp>
#include <silkworm/stagedsync/common.hpp>
#include <silkworm/consensus/noproof/engine.hpp>

class mock_receiver_plugin_impl : std::enable_shared_from_this<mock_receiver_plugin_impl> {
   public:
      mock_receiver_plugin_impl()
         : blocks_channel(appbase::app().get_channel<channels::blocks>()) {
      }

      inline void init() {
      }

      inline void startup() {
         
         for (int i = 0; i < 10; i++) {
            auto b = construct_block(i, {});
            blocks_channel.publish(priority, std::make_shared<channels::block>(b));
         }
         blocks_channel.publish(priority, std::make_shared<channels::block>(generate_block_1()));
      }

      inline void shutdown() {
      }

      void execute_block() {
      }

      channels::block generate_block_1() {
         channels::block block;

         block.header.difficulty = 0x200000;
         block.header.gas_limit  = std::numeric_limits<uint64_t>::max();
         block.header.number     = 0x1;
         block.header.timestamp  = 0x03e8;
         block.header.parent_hash = evmc_bytes32{0x5f, 0xda, 0x72, 0x7e, 0x2d, 0x76, 0x8f, 0xb7, 0x1e, 0x32, 0xa4, 0xf0, 0x47,
                                                 0x19, 0x53, 0x9f, 0xd2, 0xa7, 0xdf, 0xd7, 0x80, 0xec, 0x16, 0x95, 0xc7, 0xf3,
                                                 0xec, 0x26, 0x90, 0xbe, 0xf4, 0x2b};
         block.header.receipts_root = evmc_bytes32{0xba, 0x99, 0x91, 0xe5, 0x73, 0x82, 0x1b, 0x0c, 0x23, 0x4e, 0x36, 0x1e, 0x68,
                                                   0xd2, 0xfe, 0x4b, 0x0f, 0xc9, 0x40, 0xf8, 0x38, 0x0d, 0x61, 0xfc, 0x18, 0xee,
                                                   0x6c, 0xeb, 0x96, 0x3b, 0x70, 0x94};
         block.header.state_root   = evmc_bytes32{0xb5, 0x17, 0x7b, 0x68, 0x95, 0xf2, 0xe0, 0x82, 0xbb, 0xdb, 0x97, 0x70, 0xe2,
                                                  0x87, 0x9d, 0xcb, 0x0f, 0xd3, 0x41, 0x1a, 0x11, 0xdd, 0x32, 0xb7, 0xeb, 0x3e,
                                                  0x4e, 0xf9, 0x00, 0xbd, 0x92, 0x45};
         block.header.transactions_root = evmc_bytes32{0xf1, 0x25, 0x0d, 0x5b, 0xdb, 0xf3, 0x6e, 0x38, 0x6b, 0x5c, 0x0a, 0xbb, 0xd3,
                                                       0x4f, 0x2d, 0xba, 0x30, 0xa8, 0x33, 0xe5, 0x7f, 0x9a, 0x85, 0xad, 0x83, 0xc1,
                                                       0x0a, 0x13, 0xff, 0x61, 0xf8, 0xea};
         //block.header.ommers_hash  = evmc_bytes32{0x1d, 0xcc, 0xc4, 0xde, 0x8d, 0xec, 0x75, 0xd7, 0xaa, 0xb8, 0x5b, 0x56, 0x7b, 0x6c,
         //                                         0xcd, 0x41, 0xad, 0x31, 0x24, 0x51, 0xb9, 0x48, 0xa7, 0x41, 0x3f, 0x0a, 0x14, 0x2f,
         //                                         0xd4, 0x0d, 0x49, 0x347"; 
         silkworm::Bytes txn = {0xf9, 0x02, 0x68, 0xf9, 0x01, 0xfd, 0xa0, 0x5f, 0xda, 0x72, 0x7e, 0x2d, 0x76, 0x8f, 0xb7, 0x1e, 0x32, 0xa4, 0xf0, 0x47, 0x19, 0x53, 0x9f, 0xd2, 0xa7, 0xdf, 0xd7, 0x80, 0xec, 0x16, 0x95, 0xc7, 0xf3, 0xec, 0x26, 0x90, 0xbe, 0xf4, 0x2b, 0xa0, 0x1d, 0xcc, 0x4d, 0xe8, 0xde, 0xc7, 0x5d, 0x7a, 0xab, 0x85, 0xb5, 0x67, 0xb6, 0xcc, 0xd4, 0x1a, 0xd3, 0x12, 0x45, 0x1b, 0x94, 0x8a, 0x74, 0x13, 0xf0, 0xa1, 0x42, 0xfd, 0x40, 0xd4, 0x93, 0x47, 0x94, 0x94, 0x53, 0x04, 0xeb, 0x96, 0x06, 0x5b, 0x2a, 0x98, 0xb5, 0x7a, 0x48, 0xa0, 0x6a, 0xe2, 0x8d, 0x28, 0x5a, 0x71, 0xb5, 0xa0, 0xb5, 0x17, 0x7b, 0x68, 0x95, 0xf2, 0xe0, 0x82, 0xbb, 0xdb, 0x97, 0x70, 0xe2, 0x87, 0x9d, 0xcb, 0x0f, 0xd3, 0x41, 0x1a, 0x11, 0xdd, 0x32, 0xb7, 0xeb, 0x3e, 0x4e, 0xf9, 0x00, 0xbd, 0x92, 0x45, 0xa0, 0xf1, 0x25, 0x0d, 0x5b, 0xdb, 0xf3, 0x6e, 0x38, 0x6b, 0x5c, 0x0a, 0xbb, 0xd3, 0x4f, 0x2d, 0xba, 0x30, 0xa8, 0x33, 0xe5, 0x7f, 0x9a, 0x85, 0xad, 0x83, 0xc1, 0x0a, 0x13, 0xff, 0x61, 0xf8, 0xea, 0xa0, 0xba, 0x99, 0x91, 0xe5, 0x73, 0x82, 0x1b, 0x0c, 0x23, 0x4e, 0x36, 0x1e, 0x68, 0xd2, 0xfe, 0x4b, 0x0f, 0xc9, 0x40, 0xf8, 0x38, 0x0d, 0x61, 0xfc, 0x18, 0xee, 0x6c, 0xeb, 0x96, 0x3b, 0x70, 0x94, 0xb9, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x02, 0x00, 0x00, 0x01, 0x88, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x83, 0x06, 0x1a, 0x80, 0x82, 0x03, 0xe8, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x65, 0xf8, 0x63, 0x80, 0x0a, 0x83, 0x06, 0x1a, 0x80, 0x94, 0x09, 0x5e, 0x7b, 0xae, 0xa6, 0xa6, 0xc7, 0xc4, 0xc2, 0xdf, 0xeb, 0x97, 0x7e, 0xfa, 0xc3, 0x26, 0xaf, 0x55, 0x2d, 0x87, 0x83, 0x01, 0x86, 0xa0, 0x42, 0x1c, 0xa0, 0xb8, 0xdc, 0x57, 0x21, 0x0e, 0x10, 0x10, 0x7f, 0x8f, 0xca, 0x7b, 0x19, 0x42, 0xbb, 0x89, 0x14, 0xb6, 0xf3, 0x6c, 0xfe, 0xff, 0x54, 0xd2, 0xa9, 0xa0, 0xff, 0x45, 0xf0, 0x3c, 0x66, 0x08, 0x68, 0xa0, 0x78, 0xaa, 0xfb, 0x7b, 0xb6, 0x32, 0xc7, 0x17, 0x06, 0x7f, 0x91, 0x65, 0xcd, 0x30, 0xc5, 0xd3, 0x7b, 0x4e, 0x07, 0x13, 0xcd, 0xc0, 0x1b, 0x44, 0x60, 0x73, 0xab, 0xb8, 0x37, 0x41, 0x3a, 0x78, 0xc0};
         silkworm::Transaction t;
         silkworm::ByteView tv = {txn.data(), txn.size()};
         auto res = silkworm::rlp::decode(tv, t);
         SILK_INFO << "DECODED : " << std::string(magic_enum::enum_name<silkworm::DecodingResult>(res));
         return block;
      }

      channels::block construct_block(uint32_t block_num, std::vector<eosio::ship_protocol::signed_block_v0> blocks) {
         channels::block block;
         block.header.difficulty = 0;
         block.header.gas_limit  = std::numeric_limits<uint64_t>::max();
         block.header.number     = block_num;
         return block;
      }

      static inline constexpr std::size_t priority = 80;
      channels::blocks::channel_type&     blocks_channel;

};

mock_receiver_plugin::mock_receiver_plugin() : my(new mock_receiver_plugin_impl()) {}
mock_receiver_plugin::~mock_receiver_plugin() {}

void mock_receiver_plugin::set_program_options( appbase::options_description& cli, appbase::options_description& cfg ) {
   cfg.add_options()
      ("chain-data", boost::program_options::value<std::string>()->default_value("."),
        "chain data path as a string")
   ;
}

void mock_receiver_plugin::plugin_initialize( const appbase::variables_map& options ) {
   my->init();
   SILK_INFO << "Initialized mock_receiver Plugin";
}

void mock_receiver_plugin::plugin_startup() {
   my->startup();
   SILK_INFO << "Started mock_receiver plugin";
}

void mock_receiver_plugin::plugin_shutdown() {
   my->shutdown();
   SILK_INFO << "Shutdown mock_receiver plugin";
}