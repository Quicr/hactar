# QChat - QuicR Chat messaging Protocol (v0.1)

This is a very rough sketch of building a chat protocol using QuicR as transport.

# Protocol

## Model 

Here is the model descrbing actors and their relationships when 
exceuting an instance of the QChat Protocol.

~~~
                   +-------------+          is part of
                   |             |
             +---->|    Org      |<---------------------+
             |     |             |                      |
             |     +-------------+                      |
             |                                          |
  belongs    |                                          |
     to      |                                   +-------------+
             |                                   |             |
             |                                   |   Channel   |
             |                                   |             |
             |                                   +-------------+
      +-------------+                                   |
      |             |                                   |
      |    User     |                                   |   has 1:N
      |             |                                   |
      +-------------+                                   v
             |                                   +-------------+
             |                                   |             |
 has 1:N     |                                   |   Room      |
             |                                   |             |
             v                                   +-------------+
      +-------------+
      |             |
      |   Device    |
      |             |
      +-------------+
~~~

TODO: Define policies for rooms and channels.

Based on the model depicted above, for a given active QuicR messaging protocol instantation:

1. Each user belongs to a single organization and can have multiple devices actively participating at a given point in time.

3. Each organization has one or more Channels that represents the contianment of 
one or more Rooms.

4. User's devices are added to a Channel and can access any room in that channel.

5. Users participate in message exchange by joining a room under a channel.

6. Each message gets a unique name and is published at that name. Members of the room subscribe to a wildcarded subset of the name to receive the desired messages.


**Sample Call Flow**
Here is the basic call flow showing Alice and Bob participating in the QuicR Chat protocol for joining a Room called "cafe" and exchanging chat message.

~~~mermaid

sequenceDiagram
    participant Alice
    participant Relay
    participant Bob
    Note over Alice, Bob: Information on org <br/>channels, rooms, device information is obtained already

    Note left of Alice: Subscribe to cafe room
    Alice ->> Relay: subscribe quicr://chat/<....>/room/cafe/
    Relay ->> Alice: subscribe ok

    Note right of Bob: Subscribe to cafe room
    Alice ->> Relay: subscribe qchat://<....>/room/cafe/
    Relay ->> Bob: subscribe ok
    
    Note over Alice, Bob: Alice and Bob chat exchange
    Note left of Alice: Alice sends message "hi Bob" <br/>to the cafe room
    Alice ->> Relay: publish qchat://<....>/room/cafe/endpoint/alice-ep/msg/1, "hi Bob"
    Relay ->> Bob: publish qchat://<....>/room/cafe/device/alice-dev/msg/1, "hi Bob"
    note right of Bob: Message from Alice is displayed, Bob responds
    Bob-->>Relay: publish qchat://<....>/room/cafe/endpoint/bob-ep/msg/1, "hello alice"
    Relay-->>Alice: publish qchat://<....>/room/cafe/endpoint/bob-ep/msg/1, "hello alice"
    note left of Alice: Message from Bob is displayed,
~~~



## Message Names

Each message being published has a unique QuicR name, whose template is as defined below:

```
quicr://origin/<int24>/version/<int8>/appId/<int8>/org/<int12>/channels/<int24>/room/<int8>/endpoint/<int16>/message/<int32>

```

where:
* origin        (24 bits): DNS domain name of origin server
* version       (8 bits) : protocol version number 
* appID         (8 bits) : Application ID (ex: chat)
* org           (12 bits): number allocated by the origin domain for each organization using
                           this origin
* channel       (20 bits): number allocated by org for each team in the org 
* room          (16 bits): number allocated by team owner for each channel in the team
* endpoint      (20 bits): number unique to the team for the tuple <user, device> for the channel.
* msgNum        (20 bits): number allocated by the device for each message in this
                           channel from this device


A QuicR name represented as uri is compressed into 128 bit integer. Each message is published with a unique QuicR Name, called "MessageId (msgId)" formed by concatenating the above.

"RoomID (roomId)" is formed by concatenating above with device set to 0 
and msgNum set to 0

Devices publish messages to msgId and receive messages by
subscribing to a roomId with 40 bit wildcard (for endpoint and message)

**Name Example**

For the message name represented as a URI where the **endpoint 1** is publishing to the **room cafe** is as shown below

```
Pubish quicr://origin/1/version/1/appId/1/org/1/channel/CB5/room/CAFE/endpoint/1/msg/1 
```

and the  corresponding compressed 128 bit integer for the same is shown below
```
Publish 0x000001010100100CB5CAFE0000100001
```


Below shows an example of subscribing to the room cafe 

```
Subscribe 0x000001010100100CB5CAFE0000000000/40
```


## Default Channel

Every org has a default channel identified by its quicr name :

```
quicr://origin/1/version/1/appId/1/org/1/channel/channel-0
```

All the devices are authorized to be members of the default channel
as part of user/device onboarding. Devices subscribe to the default
channel on bootup to receive udpates on new channels made available.


TODO: I can see cases where there can be more than one default-channel, but something to worry for later.


## Default Room
Every channel has a default room identified by its quicr name :

```
quicr://origin/1/version/1/appId/1/org/1/channel/<ch-id>/room/room-0
```
Devices subscribe to the default room to get updates on the rooms and management of the rooms.

## Boostrapping

User has the following information on bootup configured either statitically on the device or retrieved from the authorized configuration server:

* orgId
* deviceId
* user-name
* credential information
* quicr namespaces for default channels for various services (chat, meetings, 
   telemetry, etc)


The devices goes into following states to bootstrap a QChat session

~~~mermaid
flowchart TD
    A[Start] --> |Sub:Default QChat Channel| B[BootstrapPending]
    B -->|onSubscribeOk| C[InSession]
    B -->|onSubscribeError| E[End]
    C --> |onSubscribe:default-room|D[Pending]
    D --> |onSubsrcibeOk: default-room| C
    D --> |onSubsrcibeError: default-room| E[End]

~~~



## Messages


### Ascii Message

Allows ASCII characters 0x20 to 0x7E. Note this does not include CR or LF so messages meant to display on different lines need to be sent as multiple messages.

**Envelope Data** 
* message_type: Ascii_Message (1)
* publish_uri (128 bit): quicr uri for the publisher of the message 
* expiry_time (32 bit in ms since unix epoch)
* creation_time (48 bit in ms since epoch) 

**Payload Data**
* reply_to: optional reply to device/msg 
* replaces: optional replaces device/msg 
* message: message entered by the user 


### Watch and Unwatch Rooms
These messages are used to listen to and stop listening for messages from a given room


**Envelope Data** 
* message_type: Watch (2) / Unwatch (3)
* publish_uri (128 bit): quicr uri for the publisher of the message 

**Payload Data**
* channel: channel identifier 
* room: room identifier


## TODO
Once we have a basic prototype, work on v0.2
- State persistence across reboots and network connectivity issues
- 
