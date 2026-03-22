#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio.hpp>

#include "clearspace/core/server_core.hpp"
#include "clearspace/persistence/snapshot_writer.hpp"
#include "clearspace/persistence/sqlite_snapshot_store.hpp"
#include "clearspace/server/tcp_server.hpp"

int main(int argc, char* argv[]) {
  try {
    unsigned short port = 5555;
    if (argc > 1) {
      port = static_cast<unsigned short>(std::stoi(argv[1]));
    }

    std::string dbPath = "clearspace_snapshots.db";
    if (argc > 2) {
      dbPath = argv[2];
    }

    boost::asio::io_context ioContext;

    auto store = std::make_unique<clearspace::persistence::SqliteSnapshotStore>(dbPath);
    auto writer = std::make_shared<clearspace::persistence::SnapshotWriter>(std::move(store));
    auto core = std::make_shared<clearspace::core::ServerCore>(clearspace::core::makeSystemTimeProvider(), writer);

    clearspace::server::TcpServer server(
        ioContext,
        clearspace::server::TcpServer::tcp::endpoint(clearspace::server::TcpServer::tcp::v4(), port),
        core);
    server.start();

    std::vector<std::thread> workers;
    const unsigned int threadsCount = std::max(2u, std::thread::hardware_concurrency());
    workers.reserve(threadsCount);
    for (unsigned int i = 0; i < threadsCount; ++i) {
      workers.emplace_back([&ioContext] { ioContext.run(); });
    }

    for (auto& worker : workers) {
      worker.join();
    }
  } catch (const std::exception& ex) {
    std::cerr << "Server failed: " << ex.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
