#ifndef UNIX_FCITX5_MOZC_CLIENT_HOLDER_H_
#define UNIX_FCITX5_MOZC_CLIENT_HOLDER_H_

#include "client/client_interface.h"
#include "unix/fcitx5/mozc_client_interface.h"

namespace fcitx {

class MozcClientHolder : public MozcClientInterface {
 public:
  MozcClientHolder();

  MozcClientHolder(MozcClientHolder &&) = delete;

  ~MozcClientHolder();

  bool EnsureConnection() override;
  bool SendKeyWithContext(const mozc::commands::KeyEvent &key,
                          const mozc::commands::Context &context,
                          mozc::commands::Output *output) override;
  bool SendCommandWithContext(const mozc::commands::SessionCommand &command,
                              const mozc::commands::Context &context,
                              mozc::commands::Output *output) override;
  bool IsDirectModeCommand(const mozc::commands::KeyEvent &key) const override;
  bool GetConfig(mozc::config::Config *config) override;
  void set_client_capability(
      const mozc::commands::Capability &capability) override;
  bool SyncData() override;
  bool LaunchTool(const std::string &mode, absl::string_view arg) override;
  bool LaunchToolWithProtoBuf(const mozc::commands::Output &output) override;

 private:
  std::unique_ptr<mozc::client::ClientInterface> client_;
};

}  // namespace fcitx
#endif
