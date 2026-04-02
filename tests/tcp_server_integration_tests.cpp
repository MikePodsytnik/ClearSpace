#include <chrono>
#include <string>
#include <thread>

#include <boost/asio.hpp>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "clearspace/core/server_core.hpp"
#include "clearspace/server/tcp_server.hpp"

namespace {
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

std::string readLine(tcp::socket& socket) {
  boost::asio::streambuf buffer;
  boost::asio::read_until(socket, buffer, '\n');
  std::istream in(&buffer);
  std::string line;
  std::getline(in, line);
  return line;
}

void sendLine(tcp::socket& socket, const std::string& line) {
  boost::asio::write(socket, boost::asio::buffer(line + "\n"));
}

class ServerFixture : public ::testing::Test {
 protected:
  void SetUp() override {
    core = std::make_shared<clearspace::core::ServerCore>();
    server = std::make_unique<clearspace::server::TcpServer>(
        ioContext,
        tcp::endpoint(tcp::v4(), 0),
        core);
    server->start();
    worker = std::thread([this] { ioContext.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  void TearDown() override {
    ioContext.stop();
    if (worker.joinable()) {
      worker.join();
    }
  }

  tcp::socket connectClient() {
    tcp::socket socket(ioContextClient);
    socket.connect(tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), server->port()));
    return socket;
  }

  boost::asio::io_context ioContext;
  boost::asio::io_context ioContextClient;
  std::shared_ptr<clearspace::core::ServerCore> core;
  std::unique_ptr<clearspace::server::TcpServer> server;
  std::thread worker;
};

}

TEST_F(ServerFixture, RespondsToPing) {
  auto socket = connectClient();
  sendLine(socket, R"({"type":"ping"})");
  const auto response = json::parse(readLine(socket));
  EXPECT_EQ(response.at("type").get<std::string>(), "pong");
}

TEST_F(ServerFixture, CreatesRoomAndReturnsSnapshotOnJoin) {
  auto socket = connectClient();
  sendLine(socket, R"({"type":"create_room"})");
  const auto created = json::parse(readLine(socket));
  ASSERT_EQ(created.at("type").get<std::string>(), "room_created");
  ASSERT_EQ(created.at("room").get<std::uint64_t>(), 1U);

  sendLine(socket, R"({"type":"join_room","room":1})");
  const auto snapshot = json::parse(readLine(socket));
  EXPECT_EQ(snapshot.at("type").get<std::string>(), "room_snapshot");
  EXPECT_EQ(snapshot.at("room").get<std::uint64_t>(), 1U);
}

TEST_F(ServerFixture, BroadcastsEventsToOtherMembersAndReportsDisconnect) {
  auto first = connectClient();
  auto second = connectClient();

  sendLine(first, R"({"type":"create_room"})");
  readLine(first);

  sendLine(first, R"({"type":"join_room","room":1})");
  readLine(first);

  sendLine(second, R"({"type":"join_room","room":1})");
  readLine(second);

  sendLine(first, R"({"type":"stroke_begin","room":1,"color":1,"thickness":1,"start":{"x":1,"y":2}})");
  const auto event = json::parse(readLine(second));
  EXPECT_EQ(event.at("type").get<std::string>(), "stroke_begin");
  EXPECT_EQ(event.at("author").get<std::uint64_t>(), 1U);

  boost::system::error_code ignored;
  first.shutdown(tcp::socket::shutdown_both, ignored);
  first.close(ignored);

  const auto msg1 = json::parse(readLine(second));
  const auto msg2 = json::parse(readLine(second));

  ASSERT_EQ(msg1.at("type").get<std::string>(), "stroke_end");
  ASSERT_EQ(msg2.at("type").get<std::string>(), "member_left");
  EXPECT_EQ(msg2.at("client").get<std::uint64_t>(), 1U);
}
