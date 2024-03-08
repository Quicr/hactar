#include <cantina/logger.h>
#include <mls/state.h>
#include <quicr/quicr_client.h>
#include <quicr/quicr_common.h>
#include <transport/transport.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>
#include <thread>

using namespace mls;

static const CipherSuite cipher_suite = CipherSuite{
  CipherSuite::ID::P256_AES128GCM_SHA256_P256
};

static const bytes group_id = from_ascii("group_id");

enum struct MlsMessageType : uint8_t {
    key_package = 1,
    welcome = 2,
    commit = 3,
    message = 4,
};

struct MLSState
{
    bool should_commit() const
    {
        return state.index() == LeafIndex{ 0 };
    }

    std::tuple<bytes, bytes> add(const bytes& key_package_data)
    {
        const auto fresh_secret = random_bytes(32);
        const auto key_package = tls::get<KeyPackage>(key_package_data);
        const auto add = state.add_proposal(key_package);
        auto [commit, welcome, next_state] =
            state.commit(fresh_secret, CommitOpts{ { add }, true, false, {} }, {});

        state = std::move(next_state);
        return { tls::marshal(commit), tls::marshal(welcome) };
    }

    void handle(const bytes& commit_data)
    {
        const auto commit = tls::get<MLSMessage>(commit_data);
        auto maybe_next_state = state.handle(commit);
        state = std::move(maybe_next_state.value());
    }

    bytes protect(const bytes& plaintext)
    {
        const auto private_message = state.protect({}, plaintext, 0);
        return tls::marshal(private_message);
    }

    bytes unprotect(const bytes& ciphertext)
    {
        const auto private_message = tls::get<MLSMessage>(ciphertext);
        const auto [aad, pt] = state.unprotect(private_message);
        return pt;
    }

  mls::State state;
};

struct PreJoinedState
{
    SignaturePrivateKey identity_priv;
    HPKEPrivateKey init_priv;
    HPKEPrivateKey leaf_priv;
    LeafNode leaf_node;
    KeyPackage key_package;
    bytes key_package_data;

    PreJoinedState(const std::string& username)
        : identity_priv(SignaturePrivateKey::generate(cipher_suite))
        , init_priv(HPKEPrivateKey::generate(cipher_suite))
        , leaf_priv(HPKEPrivateKey::generate(cipher_suite))
    {
        auto credential = Credential::basic(from_ascii(username));
        leaf_node = LeafNode{ cipher_suite,
                                leaf_priv.public_key,
                                identity_priv.public_key,
                                credential,
                                Capabilities::create_default(),
                                Lifetime::create_default(),
                                {},
                                identity_priv };

        key_package = KeyPackage{ cipher_suite,
                                    init_priv.public_key,
                                    leaf_node,
                                    {},
                                    identity_priv };

        key_package_data = tls::marshal(key_package);
    }

    MLSState create()
    {
        return { State{ group_id,
                        cipher_suite,
                        leaf_priv,
                        identity_priv,
                        leaf_node,
                        {} } };
    }

    MLSState join(const bytes& welcome_data)
    {
        const auto welcome = tls::get<Welcome>(welcome_data);
        return MLSState{ State{ init_priv,
                                leaf_priv,
                                identity_priv,
                                key_package,
                                welcome,
                                std::nullopt,
                                {} } };
    }
};

static bytes frame(MlsMessageType msg_type, const bytes& msg) {
  const auto msg_type_8 = static_cast<uint8_t>(msg_type);
  const auto type_vec = std::vector<uint8_t>(1, msg_type_8);
  return bytes(type_vec) + msg;
}

static std::pair<MlsMessageType, bytes> unframe(const bytes& framed) {
  const auto& data = framed.as_vec();
  const auto msg_type = static_cast<MlsMessageType>(data.at(0));
  const auto msg_data = bytes(std::vector<uint8_t>(data.begin() + 1, data.end()));
  return { msg_type, msg_data };
}

static std::optional<PreJoinedState> pre_joined_state = std::nullopt;
static std::optional<MLSState> mls_state = std::nullopt;

class SubDelegate : public quicr::SubscriberDelegate
{
  public:
    explicit SubDelegate(cantina::LoggerPointer &logger, quicr::Client& c)
        : logger(std::make_shared<cantina::Logger>("SDEL", logger)), client{c}
    {
    }

    void onSubscribeResponse(const quicr::Namespace &quicr_namespace, const quicr::SubscribeResult &result) override
    {
        LOGGER_INFO(logger, "onSubscriptionResponse: ns=" << quicr_namespace << " status=" << static_cast<unsigned>(result.status));
    }

    void onSubscriptionEnded(const quicr::Namespace &quicr_namespace,
                             [[maybe_unused]] const quicr::SubscribeResult::SubscribeStatus &reason) override
    {
        LOGGER_INFO(logger, "onSubscriptionEnded: name: " << quicr_namespace);
    }

    void onSubscribedObject(const quicr::Name &name,
                            [[maybe_unused]] uint8_t priority,
                            [[maybe_unused]] uint16_t expiry_age_ms,
                            [[maybe_unused]] bool use_reliable_transport,
                            quicr::bytes &&data) override
    try
    {
        LOGGER_INFO(logger, "recv object: name: " << name << " data sz: " << data.size());

        if (data.empty())
        {
            return;
        }

        const auto [msg_type, msg_data] = unframe(data);

        LOGGER_INFO(logger, "[MLS] Msg Type: " << static_cast<int>(msg_type));
        LOGGER_INFO(logger, "[MLS] Msg Data: " << msg_data);

        switch (msg_type) {
            case MlsMessageType::key_package: {
                // If this is the initial creation, create the group
                if (pre_joined_state) {
                    auto state = pre_joined_state->create();
                    const auto [commit, welcome] = state.add(msg_data);

                    pre_joined_state = std::nullopt;
                    mls_state = std::move(state);

                    auto framed_welcome = frame(MlsMessageType::welcome, welcome);
                    client.publishNamedObject(name, 0, 1000, false, std::move(framed_welcome));
                    break;
                }

                // Otherwise, we should have MLS state ready
                if (!mls_state)
                {
                    LOGGER_ERROR(logger, "MLS State not set");
                    break;
                }

                if (!mls_state->should_commit())
                {
                    // We are not the committer
                    LOGGER_INFO(logger, "Not the committer, do nothing");
                    break;
                }

                const auto [commit, welcome] = mls_state->add(msg_data);

                auto framed_welcome = frame(MlsMessageType::welcome, welcome);
                client.publishNamedObject(name, 0, 1000, false, std::move(framed_welcome));

                auto framed_commit = frame(MlsMessageType::commit, commit);
                client.publishNamedObject(name, 0, 1000, false, std::move(framed_commit));
                break;
            }

            case MlsMessageType::welcome: {
                if (!pre_joined_state) {
                    // Can't join by welcome
                    LOGGER_ERROR(logger, "[MLS] Ignoring Welcome; not pre-joined");
                    break;
                }

                LOGGER_DEBUG(logger, "[MLS] Joining");
                mls_state = pre_joined_state->join(msg_data);
                pre_joined_state = std::nullopt;
                break;
            }

            case MlsMessageType::commit: {
                if (!mls_state) {
                    // Can't handle commits before join
                    LOGGER_ERROR(logger, "[MLS] Ignoring Commit; no MLS state");
                    break;
                }

                LOGGER_DEBUG(logger, "[MLS] Processing commit");
                mls_state->handle(msg_data);
                break;
            }

            case MlsMessageType::message: {
                // If we don't have MLS state, we can't decrypt
                if (!mls_state) {
                    LOGGER_ERROR(logger, "[MLS] Ignoring message; no MLS state");
                    LOGGER_INFO(logger, "[decryption failure]");
                    break;
                }

                LOGGER_DEBUG(logger, "[MLS] Decrypting message");
                auto plaintext = mls_state->unprotect(msg_data);
                auto plaintext_str = std::string(to_ascii(plaintext));
                LOGGER_INFO(logger, plaintext_str);
                break;
            }
            default:
                break;
        }
    }
    catch (const std::exception& e)
    {
        LOGGER_ERROR(logger, "Caught exception: " << e.what());
    }

    void onSubscribedObjectFragment([[maybe_unused]] const quicr::Name &quicr_name,
                                    [[maybe_unused]] uint8_t priority,
                                    [[maybe_unused]] uint16_t expiry_age_ms,
                                    [[maybe_unused]] bool use_reliable_transport,
                                    [[maybe_unused]] const uint64_t &offset,
                                    [[maybe_unused]] bool is_last_fragment,
                                    [[maybe_unused]] quicr::bytes &&data) override
    {
    }

  private:
    cantina::LoggerPointer logger;
    quicr::Client& client;
};

class PubDelegate : public quicr::PublisherDelegate
{
  public:
    explicit PubDelegate(cantina::LoggerPointer &logger) : logger(std::make_shared<cantina::Logger>("PDEL", logger))
    {
    }

    void onPublishIntentResponse(const quicr::Namespace &quicr_namespace,
                                 const quicr::PublishIntentResult &result) override
    {
        LOGGER_INFO(logger, "Received PublishIntentResponse for " << quicr_namespace << ": " << static_cast<int>(result.status));
    }

  private:
    cantina::LoggerPointer logger;
};

static cantina::LoggerPointer logger = std::make_shared<cantina::Logger>("moctar");

int main(int argc, char** argv)
try
{
    if (argc < 4)
    {
        LOGGER_ERROR(logger, "Relay address, port, and name must be provided");
        LOGGER_ERROR(logger, "Usage moctar 127.0.0.1 1234 FF0001");
        return EXIT_FAILURE;
    }

    const std::string relay_ip = argv[1];
    const int port = std::stoi(std::string(argv[2]));
    const quicr::Name name = std::string_view(argv[3]);

    LOGGER_INFO(logger, "Connecting to " << relay_ip << ":" << port);

    const quicr::RelayInfo relay{
        .hostname = relay_ip,
        .port = uint16_t(port),
        .proto = quicr::RelayInfo::Protocol::UDP,
    };

    const qtransport::TransportConfig tcfg{
        .tls_cert_filename = nullptr,
        .tls_key_filename = nullptr,
    };

    pre_joined_state = PreJoinedState{"mocky"};
    quicr::Client client(relay, tcfg, logger);
    if (!client.connect())
    {
        logger->Log(cantina::LogLevel::Critical, "Transport connect failed");
        return EXIT_FAILURE;
    }

    auto pd = std::make_shared<PubDelegate>(logger);
    auto sd = std::make_shared<SubDelegate>(logger, client);

    const auto nspace = quicr::Namespace(name, 84);

    LOGGER_INFO(logger, "Publish Intent for " << nspace);
    client.publishIntent(pd, nspace, {}, {}, {});

    LOGGER_INFO(logger, "Subscribe to " << nspace);
    client.subscribe(sd, nspace, quicr::SubscribeIntent::immediate, "", false, "", quicr::bytes{});

    auto framed_key_package = frame(MlsMessageType::key_package, pre_joined_state->key_package_data);
    client.publishNamedObject(name, 0, 1000, false, std::move(framed_key_package));

    while (pre_joined_state != std::nullopt)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Send Messages (Ctrl + D to exit)" << std::endl;
    std::cout << "> ";

    std::string user_entered_data;
    while(std::getline(std::cin, user_entered_data))
    {
        if (!mls_state) continue;

        const auto plaintext_data = from_ascii(user_entered_data);
        const auto ciphertext = mls_state->protect(plaintext_data);
        auto&& framed = frame(MlsMessageType::message, ciphertext);

        if (!framed.empty())
        {
            LOGGER_INFO(logger, "Publishing data");
            client.publishNamedObject(name, 0, 1000, false, std::move(framed));
        }

        std::cout << "Send Messages (Ctrl + D to exit)" << std::endl;
        std::cout << "> ";
    }

    LOGGER_INFO(logger, "Unsubscribing");
    client.unsubscribe(nspace, {}, {});

    return EXIT_SUCCESS;
}
catch(const std::exception& e)
{
    LOGGER_ERROR(logger, "Caught exception: " << e.what());
    return EXIT_FAILURE;
}
