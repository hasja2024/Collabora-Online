/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "helpers.hpp"
#include "testlog.hpp"
#include "Unit.hpp"
#include "Util.hpp"

#include <Poco/URI.h>

#include <memory>
#include <string>
#include <vector>

class UnitWebSocket final
{
    std::shared_ptr<http::WebSocketSession> _httpSocket;

public:
    /// Get a websocket connected for a given URL
    UnitWebSocket(const std::shared_ptr<SocketPoll>& socketPoll, const std::string& documentURL)
    {
        Poco::URI uri(helpers::getTestServerURI());
        _httpSocket = helpers::connectLOKit(socketPoll, uri, documentURL, "UnitWebSocket ");
    }

    /// Destroy the WS.
    /// Here, we can't do IO as we don't own the socket (SocketPoll does).
    /// In fact, we can't destroy it (it's referenced by SocketPoll).
    /// Instead, we can only flag for shutting down.
    ~UnitWebSocket() { _httpSocket->asyncShutdown(); }

    const std::shared_ptr<http::WebSocketSession>& getWebSocket() { return _httpSocket; }
};

/// A WSD unit-test base class with support
/// to manage client connections.
class UnitWSDClient : public UnitWSD
{
public:
    UnitWSDClient(const std::string& name)
        : UnitWSD(name)
    {
    }

protected:
    const std::string& getWopiSrc() const { return _wopiSrc; }

    const std::unique_ptr<UnitWebSocket>& getWs() const { return _wsList.at(0); }

    const std::unique_ptr<UnitWebSocket>& getWsAt(int index) { return _wsList.at(index); }

    void deleteSocketAt(int index)
    {
        // Don't remove from the container, because the
        // indexes are how the test refers to them.
        std::unique_ptr<UnitWebSocket>& socket = _wsList.at(index);
        socket.reset();
    }

    void initWebsocket(const std::string& wopiName)
    {
        Poco::URI wopiURL(helpers::getTestServerURI() + wopiName + "&testname=" + getTestname());

        _wopiSrc.clear();
        Poco::URI::encode(wopiURL.toString(), ":/?", _wopiSrc);

        // This is just a client connection that is used from the tests.
        LOG_TST("Connecting test client to COOL (#" << (_wsList.size() + 1)
                                                    << " connection): /cool/" << _wopiSrc << "/ws");

        // Insert at the front.
        const auto& _ws = _wsList.emplace(
            _wsList.begin(),
            Util::make_unique<UnitWebSocket>(socketPoll(), "/cool/" + _wopiSrc + "/ws"));

        assert((*_ws).get());
    }

    void addWebSocket()
    {
        // This is just a client connection that is used from the tests.
        LOG_TST("Connecting test client to COOL (#" << (_wsList.size() + 1)
                                                    << " connection): /cool/" << _wopiSrc << "/ws");

        // Insert at the back.
        const auto& _ws = _wsList.emplace(
            _wsList.end(),
            Util::make_unique<UnitWebSocket>(socketPoll(), "/cool/" + _wopiSrc + "/ws"));

        assert((*_ws).get());
    }

private:
    /// The WOPISrc URL.
    std::string _wopiSrc;

    /// Websockets to communicate.
    std::vector<std::unique_ptr<UnitWebSocket>> _wsList;
};

/// Send a command message to WSD from a WopiTestServer on the given connection.
#define WSD_CMD_BY_CONNECTION_INDEX(INDEX, MSG)                                                    \
    do                                                                                             \
    {                                                                                              \
        LOG_TST("Sending from #" << INDEX << ": " << MSG);                                         \
        helpers::sendTextFrame(getWsAt(INDEX)->getWebSocket(), MSG, getTestname());                \
        SocketPoll::wakeupWorld();                                                                 \
    } while (false)

/// Send a command message to WSD from a WopiTestServer on the primary connection.
#define WSD_CMD(MSG) WSD_CMD_BY_CONNECTION_INDEX(0, MSG)

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
