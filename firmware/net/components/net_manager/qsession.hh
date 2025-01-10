 #pragma once

 #include <memory>
 #include <map>

 #include <UrlEncoder.h>

 #include <quicr/client.h>
 #include <quicr/name.h>

 #include "hactar_subscribe_track_handler.hh"
 #include "hactar_publish_track_handler.hh"

 // Rough notes
 // watch a room --> publish_intent with publisher_uri for the room and subscribe for room_uri
 // TODO needs to maintain a connected state, and when it connects
 // we need to send a message to the ui to inform it that quicr is ready

 class QSession
 {

 public:
   QSession(quicr::RelayInfo relay_info,
     std::shared_ptr<AsyncQueue<QuicrObject>> app_queue);
   ~QSession() = default;
   bool connect();
   // messaging.webex.com/v1/qchat/room/cafe/user/bret
   // messaging.webex.com/v1/qchat/room/cafe/user/suhas
   // messaging.webex.com/v1/qchat/room/cafe/user/fluffy
   bool publish_intent(quicr::Namespace ns);

   //messaging.webex.com/v1/qchat/room/cafe/*
   bool subscribe(quicr::Namespace ns);
   void unsubscribe(quicr::Namespace ns);

   //messaging.webex.com/v1/qchat/room/cafe/user/bret/messageId/1 "hi Guys"
   //messaging.webex.com/v1/qchat/room/cafe/user/bret/messageId/2 "how are you"
   void publish(const quicr::Name& name, quicr::bytes& data);

   // helpers
   quicr::Namespace to_namespace(const std::string& namespace_str);

 private:

   void add_uri_templates();
   std::recursive_mutex self_mutex;
   std::unique_lock<std::recursive_mutex> lock()
   {
     return std::unique_lock{ self_mutex };
   }

   std::shared_ptr<AsyncQueue<QuicrObject>> inbound_objects;

   std::atomic_bool stop = false;
   void set_app_queue(std::shared_ptr<AsyncQueue<QuicrObject>> q);
   static constexpr auto inbound_object_timeout = std::chrono::milliseconds(100);
   cantina::LoggerPointer logger = nullptr;
   std::unique_ptr<quicr::Client> client = nullptr;
   std::map<quicr::Namespace, std::shared_ptr<SubDelegate>> sub_delegates{};
   UrlEncoder url_encoder;
 };
