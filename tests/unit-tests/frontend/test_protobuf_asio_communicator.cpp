/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/frontend/protobuf_asio_communicator.h"
#include <gtest/gtest.h>

namespace mf = mir::frontend;

namespace
{
struct SessionSignalCollector
{
    SessionSignalCollector() : session_count(0)
    {
    }

    SessionSignalCollector(SessionSignalCollector const &) = delete;

    void on_new_session()
    {
        std::unique_lock<std::mutex> ul(guard);
        session_count++;
        wait_condition.notify_one();
    }

    std::mutex guard;
    std::condition_variable wait_condition;
    int session_count;
};
}
TEST(ProtobufAsioCommunicator, connection_results_in_a_session_being_created)
{
    std::string const socket_name("/tmp/mir_test_pb_asio_socket");

    SessionSignalCollector collector;
    mf::ProtobufAsioCommunicator comm(socket_name);
    comm.signal_new_session().connect(
            std::bind(
                &SessionSignalCollector::on_new_session,
                &collector));

    comm.start();

    boost::asio::io_service io_service;
    boost::asio::local::stream_protocol::socket socket(io_service);

    socket.connect(socket_name);

    std::unique_lock<std::mutex> ul(collector.guard);

    while (collector.session_count == 0)
        collector.wait_condition.wait_for(ul, std::chrono::milliseconds(50));

    EXPECT_EQ(1, collector.session_count);
}
