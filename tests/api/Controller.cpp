#include "Controller.h"
#include <UserPlaneComponent.h>
#include <SessionManager.h>
#include <InformationElementFactory.hpp>
#include <RulesUtilitiesImpl.h>
#include <Configuration.h>
#include <utils/LogDefines.h>

static std::shared_ptr<SessionManager> spSessionManager;

Controller::Controller(/* args */) 
{
  LOG_FUNC();
}

Controller::~Controller() 
{
  LOG_FUNC();  
}

int Controller::setup(json jBody) 
{
  LOG_FUNC();
  std::shared_ptr<RulesUtilities> mpRulesFactory;
  mpRulesFactory = std::make_shared<RulesUtilitiesImpl>();

  Configuration::sGTPInterface = jBody["gtpInterface"];
  Configuration::sUDPInterface = jBody["udpInterface"];
  LOG_DBG("GTP interface: {}", Configuration::sGTPInterface);
  LOG_DBG("UDP interface: {}", Configuration::sUDPInterface);

  UserPlaneComponent::getInstance().setup(mpRulesFactory, Configuration::sGTPInterface, Configuration::sUDPInterface);
  spSessionManager = UserPlaneComponent::getInstance().getSessionManager();
  return 200;
}

int Controller::createSesssion(json jBody) 
{
  LOG_FUNC();
  seid_t_ seid = jBody["seid"];
  u16 pdrIdUL = jBody["pdrIdUL"];
  u16 pdrIdDL = jBody["farIdUL"];
  u32 farIdUL = jBody["pdrIdDL"];
  u32 farIdDL = jBody["farIdDL"];
  u32 teid = jBody["teid"];
  std::string srcIPAddress= jBody["srcIPAddress"];
  std::string dstIPAddress= jBody["dstIPAddress"];
  apply_action_t_ actions;
  actions.forw = true;
  u16 dstPort = 1234;
  dstPort = 1234;
  struct in_addr src_addr;
  struct in_addr ue_ip;
  struct in_addr dst_addr;

  if(inet_aton(srcIPAddress.c_str(), &src_addr) == 0) {
    fprintf(stderr, "Invalid address\n");
    return 400;
  }
  if(inet_aton(dstIPAddress.c_str(), &dst_addr) == 0) {
    fprintf(stderr, "Invalid address\n");
    return 400;
  }
  // Create session, PDR and FAR
  auto pSession = createSession(seid);

  // Uplink.
  auto pPdrUL = createPDR(pdrIdUL, farIdUL, teid, INTERFACE_VALUE_ACCESS, src_addr, OUTER_HEADER_REMOVAL_GTPU_UDP_IPV4);
  auto pFarUL = createFAR(farIdUL, actions, INTERFACE_VALUE_CORE, OUTER_HEADER_CREATION_UDP_IPV4, dst_addr, dstPort);

  // Downlink.
  auto pPdrDL = createPDR(pdrIdDL, farIdDL, teid, INTERFACE_VALUE_CORE, dst_addr, OUTER_HEADER_REMOVAL_UDP_IPV4);
  auto pFarDL = createFAR(farIdDL, actions, INTERFACE_VALUE_ACCESS, OUTER_HEADER_CREATION_GTPU_UDP_IPV4, dst_addr, dstPort);

  // Request to BPF program.
  LOG_INF("Case: create session");
  spSessionManager->createSession(pSession);
  LOG_INF("Case: add UL PDR");
  spSessionManager->addPDR(pSession->getSeid(), pPdrUL);
  LOG_INF("Case: add UL FAR");
  spSessionManager->addFAR(pSession->getSeid(), pFarUL);
  LOG_INF("Case: add DL PDR");
  spSessionManager->addPDR(pSession->getSeid(), pPdrDL);
  LOG_INF("Case: add DL FAR");
  spSessionManager->addFAR(pSession->getSeid(), pFarDL);

  return 200;

}