#include <NetworkedClient/Client.hpp>

#include <easylogging++.h>
#include <libpiga_handshake.pb.h>
#include <login.pb.h>
#include <input.pb.h>

namespace NetworkedClient
{
    Client::Client(std::shared_ptr<PlayerManager> playerManager)
        : m_playerManager(playerManager)
    {
        m_client = enet_host_create(nullptr,
                                    1,
                                    2,
                                    0,
                                    0);

        if(m_client == nullptr)
            LOG(FATAL) << "ENet Client could not initialize correctly!";
        else
            LOG(INFO) << "ENet Client started successfully.";
    }
    Client::~Client()
    {
        if(m_serverPeer != nullptr)
        {
            enet_peer_disconnect(m_serverPeer, 0);
            ENetEvent event;
            bool force = false;
            while(enet_host_service(m_client, &event, 3000))
            {
                switch(event.type)
                {
                    case ENET_EVENT_TYPE_DISCONNECT:
                        LOG(INFO) << "Disconnected successfully.";
                        force = false;
                        break;
                    case ENET_EVENT_TYPE_RECEIVE:
                        enet_packet_destroy(event.packet);
                        break;
                    default:
                        force = true;
                        break;
                }
            }
            if(force)
            {
                enet_peer_reset(m_serverPeer);
                m_serverPeer = nullptr;
            }
        }
        enet_host_destroy(m_client);
        LOG(INFO) << "Client destroyed.";
    }
    void Client::setServerAddress(const std::string &adress)
    {
        m_serverAdress = adress;
    }
    void Client::setServerName(const std::string &name)
    {
        m_serverName = name;
    }
    void Client::setServerPort(int port)
    {
        m_serverPort = port;
    }
    void Client::setLoginStatus(Client::LoginResponse response)
    {
        m_loginStatus = response;
    }
    const std::string &Client::getServerAddress() const
    {
        return m_serverAdress;
    }
    const std::string &Client::getServerName() const
    {
        return m_serverName;
    }
    int Client::getServerPort() const
    {
        return m_serverPort;
    }
    Client::LoginResponse Client::getLoginStatus() const
    {
        return m_loginStatus;
    }
    std::shared_ptr<PlayerManager> Client::getPlayerManager() const
    {
        return m_playerManager;
    }
    void Client::connect()
    {
        ENetAddress address;
        ENetEvent event;
        m_connected = true;
        enet_address_set_host(&address, m_serverAdress.c_str());
        address.port = m_serverPort;

        LOG(INFO) << "Trying to connect to " << m_serverAdress << ":" << m_serverPort << ".";

        m_serverPeer = enet_host_connect(m_client, &address, 2, 0);
        if(m_serverPeer == nullptr)
        {
            LOG(FATAL) << "No available peers to initialize connection!";
            m_connected = false;
        }
    }
    bool Client::update()
    {
        bool dataChanged = false;
        ENetEvent event;

        while(enet_host_service(m_client, &event, 2))
        {
            switch(event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                    LOG(INFO) << "Client connected successfully to " << event.peer->address.host
                              << ":" << event.peer->address.port << ".";
                    m_connected = true;
                    dataChanged = true;
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    event.peer->data = nullptr;
                    LOG(INFO) << "Client disconnected.";
                    m_serverPeer = nullptr;
                    m_connected = false;
                    dataChanged = true;
                    break;
                case ENET_EVENT_TYPE_NONE:
                    //Nothing happened
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    dataChanged = receivePacket(event.packet, event.peer);
                    enet_packet_destroy(event.packet);
                    break;
            }
        }
        return dataChanged;
    }
    bool Client::disconnected()
    {
        return !m_connected;
    }
    void Client::login(const std::string &user, int userID, const std::string &pass)
    {
        ::LoginRequest loginPacket;

        loginPacket.set_user(user);
        if(pass != "")
            loginPacket.set_pass(pass);

        if(userID != -1)
            loginPacket.set_userid(userID);

        sendPacket(loginPacket, "LOGRQ", true);
    }
    Client::HandshakeCompletedSignal& Client::handshakeCompleted()
    {
        return m_handshakeCompleted;
    }
    void Client::sendInputPacket(unsigned int playerID, piga::GameControl control, int input)
    {
        if(m_serverPeer != nullptr)
        {
            Input inputPacket;

            inputPacket.set_playerid(playerID);
            inputPacket.set_input(input);

            ::GameControl controlEnum = inputPacket.control();

            switch(control)
            {
                case piga::ACTION:
                    controlEnum = ::GameControl::ACTION;
                    break;
                case piga::UP:
                    controlEnum = ::GameControl::UP;
                    break;
                case piga::DOWN:
                    controlEnum = ::GameControl::DOWN;
                    break;
                case piga::LEFT:
                    controlEnum = ::GameControl::LEFT;
                    break;
                case piga::RIGHT:
                    controlEnum = ::GameControl::RIGHT;
                    break;
                case piga::BUTTON1:
                    controlEnum = ::GameControl::BUTTON1;
                    break;
                case piga::BUTTON2:
                    controlEnum = ::GameControl::BUTTON2;
                    break;
                case piga::BUTTON3:
                    controlEnum = ::GameControl::BUTTON3;
                    break;
                case piga::BUTTON4:
                    controlEnum = ::GameControl::BUTTON4;
                    break;
                case piga::BUTTON5:
                    controlEnum = ::GameControl::BUTTON5;
                    break;
                case piga::BUTTON6:
                    controlEnum = ::GameControl::BUTTON6;
                    break;
            }

            inputPacket.set_control(controlEnum);

            sendPacket(inputPacket, "INPUT", true);
        }
    }
    void Client::sendPacket(google::protobuf::Message &msg, const std::string &packetID, bool reliable)
    {
        std::string buffer(packetID + msg.SerializeAsString());

        int enetFlags = 0;

        if(reliable)
            enetFlags = ENET_PACKET_FLAG_RELIABLE;

        ENetPacket *packet = enet_packet_create(&buffer[0u], buffer.length(), enetFlags);
        enet_peer_send(m_serverPeer, 1, packet);
    }
    bool Client::receivePacket(ENetPacket *packet, ENetPeer *peer)
    {
        bool dataChanged = false;
        std::string buffer(packet->data, packet->data + packet->dataLength);
        std::string messageType = buffer.substr(0, 5);
        buffer.erase(0, 5);
        if(messageType == "HANDS")
        {
            m_playerManager->clear();
            LibpigaHandshake handshake;
            handshake.ParseFromString(buffer);
            LOG(INFO) << "Received a handshake package from the pigaco installation \"" << handshake.name() << "\".";

            for(int i = 0; i < handshake.player_size(); ++i)
            {
                std::shared_ptr<piga::Player> player = std::make_shared<piga::Player>(
                            handshake.player(i).username().c_str(),
                            handshake.player(i).active(),
                            handshake.player(i).id());
                LOG(INFO) << "Adding the player \"" << player->getName() << "\" (ID: " << player->getPlayerID() << ").";
                m_playerManager->set(player, player->getPlayerID());
            }
            m_serverName = handshake.name();

            m_handshakeCompleted();
            dataChanged = true;
        }
        else if(messageType == "LOGRE")
        {
            ::LoginResponse response;
            response.ParseFromString(buffer);

            LOG(INFO) << "Received a login response packet from the server!";
            LoginResponse loginStatus = NotConnected;

            switch(response.response())
            {
                case ::LoginResponseEnum::LOGIN_SUCCESSFUL:
                    loginStatus = LoginSuccessful;
                    break;
                case ::LoginResponseEnum::WRONG_CREDENTIALS:
                    loginStatus = WrongCredentials;
                    break;
                case ::LoginResponseEnum::NO_LOGIN_POSSIBLE:
                    loginStatus = NoLoginPossible;
                    break;
                case ::LoginResponseEnum::NO_MORE_TRIES:
                    loginStatus = NoMoreTries;
                    break;
                case ::LoginResponseEnum::USER_ID_NOT_EXISTING:
                    loginStatus = UserIDNotExisting;
                    break;
                case ::LoginResponseEnum::USER_ID_ALREADY_ACTIVE:
                    loginStatus = UserIDAlreadyActive;
                    break;
                default:
                    loginStatus = Unknown;
                    break;
            }

            m_loginStatus = loginStatus;

            m_loginResponse(loginStatus);
            dataChanged = true;
        }
        return dataChanged;
    }
}

