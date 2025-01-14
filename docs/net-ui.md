Message structure for the UI-Net interface
==========================================

## Assumptions

* Serial link is globally flow-controlled
* UI chip has space to hold/decrypt/decode N frames to be mixed 
* Net chip picks frames to mix

## Overall flow

```
# Talk
UI->Net: ChannelSelect(channel_id)
Net->UI: ChannelSetup(channel_id, channel_keys)
...
## On button press
UI->Net: TalkStart(channel_id)
Repeat:
    UI->Net: AudioObject(channel_id, audio_data)*
UI->Net: TalkStop(channel_id)

# Play
Net->UI: PlayStart(channel_id, channel_keys)
Repeat:
    Net->UI: AudioObjects((channel_id, audio_data)*)
Net->UI: PlayStop(channel_id)
```

## On the wire representation

Framing:

```
len(u16) type(u8) data...
```


Messages:

```
type ChannelId = u8;
type FrameSize = u16;

struct ChannelInfo; // Keys, MoQ header info, etc.

// Unused for now
struct ChannelSelect(ChannelId);
struct ChannelSetup(ChannelId, ChannelInfo);

struct TalkStart(ChannelId);
struct TalkStop(ChannelId);

struct AudioObject {
    data_len: u16,
    data: [u8; data_len], // MoQ object
}

struct PlayStart(ChannelId); 
struct PlayStop(ChannelId);

struct AudioObjects {
    n_frames: u8,
    frames: [AudioFrame; n_frames],
}
```
