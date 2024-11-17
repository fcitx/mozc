#include "unix/fcitx5/mozc_client_holder.h"

#include "client/client.h"
#include "ipc/ipc.h"

namespace fcitx {

static mozc::IPCClientFactoryInterface *client_factory = nullptr;

MozcClientHolder::MozcClientHolder()
    : client_(mozc::client::ClientFactory::NewClient()) {
  if (!client_factory) {
    client_factory = mozc::IPCClientFactory::GetIPCClientFactory();
  }
  client_->SetIPCClientFactory(client_factory);
}

MozcClientHolder::~MozcClientHolder() {}

bool MozcClientHolder::EnsureConnection() {
  return client_->EnsureConnection();
}

bool MozcClientHolder::SendKeyWithContext(
    const mozc::commands::KeyEvent &key, const mozc::commands::Context &context,
    mozc::commands::Output *output) {
  return client_->SendKeyWithContext(key, context, output);
}

bool MozcClientHolder::SendCommandWithContext(
    const mozc::commands::SessionCommand &command,
    const mozc::commands::Context &context, mozc::commands::Output *output) {
  return client_->SendCommandWithContext(command, context, output);
}
bool MozcClientHolder::IsDirectModeCommand(
    const mozc::commands::KeyEvent &key) const {
  return client_->IsDirectModeCommand(key);
}
bool MozcClientHolder::GetConfig(mozc::config::Config *config) {
  return client_->GetConfig(config);
}
void MozcClientHolder::set_client_capability(
    const mozc::commands::Capability &capability) {
  return client_->set_client_capability(capability);
}
bool MozcClientHolder::SyncData() { return client_->SyncData(); }
bool MozcClientHolder::LaunchTool(const std::string &mode,
                                  absl::string_view arg) {
  return client_->LaunchTool(mode, arg);
}
bool MozcClientHolder::LaunchToolWithProtoBuf(
    const mozc::commands::Output &output) {
  return client_->LaunchToolWithProtoBuf(output);
}

std::unique_ptr<MozcClientInterface> createClient() {
  return std::make_unique<MozcClientHolder>();
}

}  // namespace fcitx
