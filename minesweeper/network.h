#pragma once

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOUSER
#define NOUSER
#endif
#endif

#include <enet/enet.h>

#ifdef _WIN32
#undef CloseWindow
#undef ShowCursor
#undef DrawText
#undef DrawTextEx
#undef Rectangle
#undef PlaySound
#undef near
#undef far
#undef max
#undef min
#endif

#include <iostream>
#include <vector>
#include <cstdint>
#include <deque>
#include <map>
#include <cstring>
#include <algorithm>

enum class NetRole { OFFLINE, HOST, CLIENT };

enum PacketType : uint8_t {
    PACKET_INIT,
    PACKET_CLICK,
    PACKET_RESULT,
    PACKET_CURSOR,
    PACKET_DISCONNECT // NEW: Explicit command to remove a player
};

#pragma pack(push, 1) 

struct PacketInit {
    PacketType type = PACKET_INIT;
    uint64_t seed;
    int dim;
    int size;
    int bombs;
};

struct PacketClick {
    PacketType type = PACKET_CLICK;
    size_t index;
    uint8_t action;
};

struct PacketResult {
    PacketType type = PACKET_RESULT;
    size_t index;
    uint8_t state;
};

struct PacketCursor {
    PacketType type = PACKET_CURSOR;
    uint32_t playerID;
    float x;
    float y;
    char name[16];
};

// NEW: Tiny payload to broadcast a player's removal
struct PacketDisconnect {
    PacketType type = PACKET_DISCONNECT;
    uint32_t playerID;
};

#pragma pack(pop)

enum NetEventType {
    NET_EVENT_CLIENT_CONNECTED,
    NET_EVENT_CLIENT_DISCONNECTED,
    NET_EVENT_INIT_BOARD,
    NET_EVENT_PLAYER_CLICK,
    NET_EVENT_BOARD_RESULT
};

struct NetEvent {
    NetEventType type;
    ENetPeer* peer;

    PacketInit initData;
    PacketClick clickData;
    PacketResult resultData;
};

class NetworkManager {
private:
    std::deque<NetEvent> eventQueue;

public:
    NetRole role = NetRole::OFFLINE;
    ENetHost* host = nullptr;
    ENetPeer* serverPeer = nullptr;
    std::vector<ENetPeer*> connectedClients;

    char playerName[16] = "PLAYER";

    struct CursorPos { float x, y; char name[16]; };
    std::map<uint32_t, CursorPos> remoteCursors;

    bool Initialize() {
        if (enet_initialize() != 0) return false;
        return true;
    }

    void Disconnect() {
        if (role == NetRole::CLIENT && serverPeer) {
            enet_peer_disconnect(serverPeer, 0);

            // Pump events for up to 100ms to guarantee the disconnect packet actually leaves the network card
            ENetEvent event;
            bool disconnected = false;
            for (int i = 0; i < 10; i++) {
                if (enet_host_service(host, &event, 10) > 0) {
                    if (event.type == ENET_EVENT_TYPE_RECEIVE) enet_packet_destroy(event.packet);
                    else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                        disconnected = true;
                        break;
                    }
                }
            }
            if (!disconnected) enet_peer_disconnect_now(serverPeer, 0);

        }
        else if (role == NetRole::HOST) {
            for (ENetPeer* peer : connectedClients) {
                enet_peer_disconnect(peer, 0);
            }

            // Pump events for up to 100ms to guarantee clients receive the boot packet
            ENetEvent event;
            for (int i = 0; i < 10; i++) {
                if (enet_host_service(host, &event, 10) > 0) {
                    if (event.type == ENET_EVENT_TYPE_RECEIVE) enet_packet_destroy(event.packet);
                }
            }
        }

        if (host) {
            enet_host_destroy(host);
            host = nullptr;
        }

        serverPeer = nullptr;
        connectedClients.clear();
        remoteCursors.clear();
        eventQueue.clear();
        role = NetRole::OFFLINE;
    }

    void Cleanup() {
        Disconnect();
        enet_deinitialize();
    }

    bool StartHost(uint16_t port) {
        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = port;

        host = enet_host_create(&address, 32, 2, 0, 0);
        if (host == nullptr) return false;

        role = NetRole::HOST;
        eventQueue.clear();
        remoteCursors.clear();
        return true;
    }

    void Broadcast(const void* data, size_t size, bool reliable = true) {
        if (!host || role != NetRole::HOST) return;
        uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
        ENetPacket* packet = enet_packet_create(data, size, flags);
        enet_host_broadcast(host, 0, packet);
    }

    bool ConnectClient(const char* ip, uint16_t port) {
        host = enet_host_create(nullptr, 1, 2, 0, 0);
        if (host == nullptr) return false;

        ENetAddress address;
        enet_address_set_host(&address, ip);
        address.port = port;

        serverPeer = enet_host_connect(host, &address, 2, 0);
        if (serverPeer == nullptr) return false;

        // REDUCE TIMEOUT: If a client crashes or wifi dies, boot them after 5s instead of 30s!
        enet_peer_timeout(serverPeer, 0, 2000, 5000);

        role = NetRole::CLIENT;
        eventQueue.clear();
        remoteCursors.clear();
        return true;
    }

    void SendToServer(const void* data, size_t size, bool reliable = true) {
        if (!serverPeer || role != NetRole::CLIENT) return;
        uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
        ENetPacket* packet = enet_packet_create(data, size, flags);
        enet_peer_send(serverPeer, 0, packet);
    }

    void Update() {
        if (!host) return;
        ENetEvent event;

        while (enet_host_service(host, &event, 0) > 0) {
            NetEvent ne;
            ne.peer = event.peer;

            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                if (role == NetRole::HOST) {
                    // REDUCE TIMEOUT: Boot crashed clients after 5s instead of 30s!
                    enet_peer_timeout(event.peer, 0, 2000, 5000);
                    connectedClients.push_back(event.peer);
                    ne.type = NET_EVENT_CLIENT_CONNECTED;
                    eventQueue.push_back(ne);
                }
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                HandleIncomingPacket(event.peer, event.packet);
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                if (role == NetRole::HOST) {
                    auto it = std::find(connectedClients.begin(), connectedClients.end(), event.peer);
                    if (it != connectedClients.end()) connectedClients.erase(it);

                    // Purge from Host Screen immediately
                    uint32_t droppedID = event.peer->connectID;
                    remoteCursors.erase(droppedID);

                    // BROADCAST TO ALL OTHER CLIENTS: Tell them this player left!
                    PacketDisconnect pd;
                    pd.playerID = droppedID;
                    Broadcast(&pd, sizeof(pd));

                    ne.type = NET_EVENT_CLIENT_DISCONNECTED;
                    eventQueue.push_back(ne);
                }
                else {
                    serverPeer = nullptr;
                    role = NetRole::OFFLINE;
                    remoteCursors.clear();
                }
                break;

            case ENET_EVENT_TYPE_NONE:
                break;
            }
        }
    }

    bool PollEvent(NetEvent& outEvent) {
        if (eventQueue.empty()) return false;
        outEvent = eventQueue.front();
        eventQueue.pop_front();
        return true;
    }

private:
    void HandleIncomingPacket(ENetPeer* sender, ENetPacket* packet) {
        if (packet->dataLength < sizeof(PacketType)) return;

        PacketType type = *(PacketType*)packet->data;
        NetEvent ev;
        ev.peer = sender;

        if (type == PACKET_INIT && role == NetRole::CLIENT) {
            ev.type = NET_EVENT_INIT_BOARD;
            ev.initData = *(PacketInit*)packet->data;
            eventQueue.push_back(ev);
        }
        else if (type == PACKET_CLICK && role == NetRole::HOST) {
            ev.type = NET_EVENT_PLAYER_CLICK;
            ev.clickData = *(PacketClick*)packet->data;
            eventQueue.push_back(ev);
        }
        else if (type == PACKET_RESULT && role == NetRole::CLIENT) {
            ev.type = NET_EVENT_BOARD_RESULT;
            ev.resultData = *(PacketResult*)packet->data;
            eventQueue.push_back(ev);
        }
        else if (type == PACKET_DISCONNECT && role == NetRole::CLIENT) {
            // NEW: Instantly delete a player's cursor when the Host tells us they left!
            PacketDisconnect* pd = (PacketDisconnect*)packet->data;
            remoteCursors.erase(pd->playerID);
        }
        else if (type == PACKET_CURSOR) {
            PacketCursor* pc = (PacketCursor*)packet->data;

            if (role == NetRole::HOST) {
                uint32_t senderID = sender->connectID;
                remoteCursors[senderID].x = pc->x;
                remoteCursors[senderID].y = pc->y;

#ifdef _WIN32
                strncpy_s(remoteCursors[senderID].name, pc->name, _TRUNCATE);
#else
                strncpy(remoteCursors[senderID].name, pc->name, 15);
                remoteCursors[senderID].name[15] = '\0';
#endif

                PacketCursor forwardPack = *pc;
                forwardPack.playerID = senderID;

                for (ENetPeer* p : connectedClients) {
                    if (p != sender) {
                        ENetPacket* fPacket = enet_packet_create(&forwardPack, sizeof(PacketCursor), 0);
                        enet_peer_send(p, 0, fPacket);
                    }
                }
            }
            else if (role == NetRole::CLIENT) {
                remoteCursors[pc->playerID].x = pc->x;
                remoteCursors[pc->playerID].y = pc->y;

#ifdef _WIN32
                strncpy_s(remoteCursors[pc->playerID].name, pc->name, _TRUNCATE);
#else
                strncpy(remoteCursors[pc->playerID].name, pc->name, 15);
                remoteCursors[pc->playerID].name[15] = '\0';
#endif
            }
        }
    }
};