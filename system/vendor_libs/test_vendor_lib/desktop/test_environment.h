//
// Copyright 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <chrono>      // for milliseconds
#include <functional>  // for __base, function
#include <future>      // for promise
#include <memory>      // for shared_ptr, make_...
#include <string>      // for string

#include "model/controller/dual_mode_controller.h"  // for DualModeController
#include "model/setup/async_manager.h"              // for AsyncTaskId, Asyn...
#include "model/setup/test_channel_transport.h"     // for TestChannelTransport
#include "model/setup/test_command_handler.h"       // for TestCommandHandler
#include "model/setup/test_model.h"                 // for TestModel
#include "net/async_data_channel_server.h"          // for AsyncDataChannelS...

namespace android {
namespace net {
class AsyncDataChannel;
class AsyncDataChannelConnector;
}  // namespace net

namespace bluetooth {
namespace root_canal {

using android::net::AsyncDataChannel;
using android::net::AsyncDataChannelConnector;
using android::net::AsyncDataChannelServer;
using android::net::ConnectCallback;

class TestEnvironment {
 public:
  TestEnvironment(std::shared_ptr<AsyncDataChannelServer> test_port,
                  std::shared_ptr<AsyncDataChannelServer> hci_server_port,
                  std::shared_ptr<AsyncDataChannelServer> link_server_port,
                  std::shared_ptr<AsyncDataChannelConnector> connector,
                  const std::string& controller_properties_file = "",
                  const std::string& default_commands_file = "")
      : test_socket_server_(test_port),
        hci_socket_server_(hci_server_port),
        link_socket_server_(link_server_port),
        connector_(connector),
        default_commands_file_(default_commands_file),
        controller_(std::make_shared<test_vendor_lib::DualModeController>(
            controller_properties_file)) {}

  void initialize(std::promise<void> barrier);

  void close();

 private:
  std::shared_ptr<AsyncDataChannelServer> test_socket_server_;
  std::shared_ptr<AsyncDataChannelServer> hci_socket_server_;
  std::shared_ptr<AsyncDataChannelServer> link_socket_server_;
  std::shared_ptr<AsyncDataChannelConnector> connector_;
  std::string default_commands_file_;
  bool test_channel_open_{false};
  std::promise<void> barrier_;

  test_vendor_lib::AsyncManager async_manager_;

  void SetUpTestChannel();
  void SetUpHciServer(ConnectCallback on_connect);
  void SetUpLinkLayerServer(ConnectCallback on_connect);
  std::shared_ptr<AsyncDataChannel> ConnectToRemoteServer(
      const std::string& server, int port);

  std::shared_ptr<test_vendor_lib::DualModeController> controller_;

  test_vendor_lib::TestChannelTransport test_channel_transport_;
  test_vendor_lib::TestChannelTransport remote_hci_transport_;
  test_vendor_lib::TestChannelTransport remote_link_layer_transport_;

  test_vendor_lib::TestModel test_model_{
      [this]() { return async_manager_.GetNextUserId(); },
      [this](test_vendor_lib::AsyncUserId user_id,
             std::chrono::milliseconds delay,
             const test_vendor_lib::TaskCallback& task) {
        return async_manager_.ExecAsync(user_id, delay, task);
      },

      [this](test_vendor_lib::AsyncUserId user_id,
             std::chrono::milliseconds delay, std::chrono::milliseconds period,
             const test_vendor_lib::TaskCallback& task) {
        return async_manager_.ExecAsyncPeriodically(user_id, delay, period,
                                                    task);
      },

      [this](test_vendor_lib::AsyncUserId user) {
        async_manager_.CancelAsyncTasksFromUser(user);
      },

      [this](test_vendor_lib::AsyncTaskId task) {
        async_manager_.CancelAsyncTask(task);
      },

      [this](const std::string& server, int port) {
        return ConnectToRemoteServer(server, port);
      }};

  test_vendor_lib::TestCommandHandler test_channel_{test_model_};
};
}  // namespace root_canal
}  // namespace bluetooth
}  // namespace android