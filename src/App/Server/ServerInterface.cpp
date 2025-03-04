#include "App/Server/ServerInterface.hpp"
#include "App/TransportParser/Read.hpp"
#include "App/Tools/Colors.hpp"
#include "App/Tools/Fixer.hpp"
#include <algorithm>
#include <iterator>
#include <thread>

app::ServerMaster::ServerMaster(const uint16_t& listenerPortClient, const uint16_t& listenerPortRepository){
  tool::ConsolePrint("[SERVER MASTER <Init Client Listener>]:", CYAN);
  if(clientListener.listen(listenerPortClient) != net::Status::Done){
    tool::ConsolePrint("=================================== (--FAILURE--)", RED);
    exit(EXIT_FAILURE);
  }
  tool::ConsolePrint("[SERVER MASTER <Init Repo. Listener>]:", CYAN);
  if(repositoryListener.listen(listenerPortRepository) != net::Status::Done){
    tool::ConsolePrint("=================================== (--FAILURE--)", RED);
    exit(EXIT_FAILURE);
  }
  tool::ConsolePrint("=================================== (--SUCCESS--)", GREEN);
  tool::ConsolePrint(">> Client Listener:", CYAN);
  std::cout << clientListener << std::endl;
  tool::ConsolePrint(">> Repository Listener:", CYAN);
  std::cout << repositoryListener << std::endl;
  tool::ConsolePrint("=================================================", VIOLET);
}

void app::ServerMaster::connEnvironmentClient(std::shared_ptr<rdt::RDTSocket> socket){
  std::string message, response;
  if(socket->online()){
    socket->receive(message);
    std::string dbNodeId;
    uint8_t commandKey = message[0];
    if(commandKey == 'c'){
      dbNodeId = message.substr(1);
      dbNodeId = tool::asStreamString(dbNodeId, 3);
      rdt::RDTSocket queryConnection;
      repositoryPoolMutex.lock();
      std::pair<uint16_t, std::string> info = repositoryConnectionPool[dbNodeId[0] % repositoryConnectionPool.size()];
      repositoryPoolMutex.unlock();
      if(queryConnection.connect(info.second, info.first) == net::Status::Done){
        if(queryConnection.online()){
          queryConnection.send(message);
          queryConnection.disconnectInitializer();
        }
      }
    }
    else if(commandKey == 'r'){
      msg::ReadNodePacket rPacket, tempPacket;
      rPacket << message;
      std::vector<std::string> givenNeighbours = {rPacket.nodeId};
      while(rPacket.depth > 0){
        std::string neighbours;
        std::stringstream buffer;
        for(std::string& id : givenNeighbours){
          rdt::RDTSocket queryConnection;
          repositoryPoolMutex.lock();
          std::pair<uint16_t, std::string> info = repositoryConnectionPool[id[0] % repositoryConnectionPool.size()];
          repositoryPoolMutex.unlock();
          if(queryConnection.connect(info.second, info.first) == net::Status::Done){
            if(queryConnection.online()){
              tempPacket << message;
              tempPacket.nodeId = id;
              tempPacket >> message;
              queryConnection.send(message);
              queryConnection.receive(neighbours);
              buffer << neighbours;
              queryConnection.receive(message);
              rPacket << message;
              queryConnection.disconnectInitializer();
            }
          }
        }
        givenNeighbours.clear();
        std::string singleNeighbour;
        while(std::getline(buffer, singleNeighbour, ',')){
          givenNeighbours.push_back(singleNeighbour);
        }
      }
      socket->send(response);
    }
    else if(commandKey == 'u'){
      dbNodeId = message.substr(2);
      dbNodeId = tool::asStreamString(dbNodeId, 2);
      for(auto& repository : repositoryConnectionPool){
        std::shared_ptr<rdt::RDTSocket> queryConnection = std::make_shared<rdt::RDTSocket>();
        if(queryConnection->connect(repository.second, repository.first) == net::Status::Done){
          std::thread queryThread([](std::shared_ptr<rdt::RDTSocket> conn, const std::string& query){
            if(conn->online()){
              conn->send(query);
              conn->disconnectInitializer();
            }
          }, queryConnection, message);
          queryThread.detach();
        }
      }
    }
    else if(commandKey == 'd'){
      dbNodeId = message.substr(2);
      dbNodeId = tool::asStreamString(dbNodeId, 2);
      for(auto& repository : repositoryConnectionPool){
        std::shared_ptr<rdt::RDTSocket> queryConnection = std::make_shared<rdt::RDTSocket>();
        if(queryConnection->connect(repository.second, repository.first) == net::Status::Done){
          std::thread queryThread([](std::shared_ptr<rdt::RDTSocket> conn, const std::string& query){
            if(conn->online()){
              conn->send(query);
              conn->disconnectInitializer();
            }
          }, queryConnection, message);
          queryThread.detach();
        }
      }
    }
    socket->passiveDisconnect();
  }
}

void app::ServerMaster::connEnvironmentRepository(std::shared_ptr<rdt::RDTSocket> socket){
  std::string queryPort;
  if(socket->online()){
    socket->receive(queryPort);
    repositoryPoolMutex.lock();
    repositoryConnectionPool.emplace_back(std::stoi(queryPort), socket->getRemoteIpAddress());
    repositoryPoolMutex.unlock();
    socket->passiveDisconnect();
  }
}

void app::ServerMaster::runRepositoryListener(){
  while(true){
    std::shared_ptr<rdt::RDTSocket> newRepositoryIntent;
    newRepositoryIntent = std::make_shared<rdt::RDTSocket>();
    alternateConsolePrintMutex.lock();
    tool::ConsolePrint("=>[SERVER MASTER <Spam>]: Waiting for a new repository...", VIOLET);
    alternateConsolePrintMutex.unlock();
    if(repositoryListener.accept(*newRepositoryIntent) == net::Status::Done){
      std::thread repositoryThread(&ServerMaster::connEnvironmentRepository, this, newRepositoryIntent);
      repositoryThread.detach();
      alternateConsolePrintMutex.lock();
      tool::ConsolePrint("=>[SERVER MASTER <Spam>]: New repository connected.", CYAN);
      std::cout << *newRepositoryIntent << std::endl;
      alternateConsolePrintMutex.unlock();
    }
  }
}

void app::ServerMaster::run(){
  std::thread repositoryThread(&ServerMaster::runRepositoryListener, this);
  while(true){
    std::shared_ptr<rdt::RDTSocket> newClientIntent;
    newClientIntent = std::make_shared<rdt::RDTSocket>();
    alternateConsolePrintMutex.lock();
    tool::ConsolePrint("=>[SERVER MASTER <Spam>]: Waiting for a new connection...", VIOLET);
    alternateConsolePrintMutex.unlock();
    if(clientListener.accept(*newClientIntent) == net::Status::Done){
      std::thread clientThread(&ServerMaster::connEnvironmentClient, this, newClientIntent);
      clientThread.detach();
      alternateConsolePrintMutex.lock();
      tool::ConsolePrint("=>[SERVER MASTER <Spam>]: New client connected.", CYAN);
      std::cout << *newClientIntent << std::endl;
      alternateConsolePrintMutex.unlock();
    }
  }
  repositoryThread.join();
}
