#ifndef UNIX_FCITX5_MOZC_CLIENT_H_
#define UNIX_FCITX5_MOZC_CLIENT_H_

#include "base/run_level.h"
#include "composer/key_event_util.h"
#include "session/session_handler.h"

namespace fcitx {
class MozcClientPool;

class MozcClient {
  friend class MozcClientPool;

 public:
  MozcClient(mozc::SessionHandler *session_handler);
  ~MozcClient();

  // Initializes `request_` with the flag.
  // This function should be called before EnsureSession.
  void InitRequestForSvsJapanese(bool use_svs);

  bool SendKeyWithContext(const mozc::commands::KeyEvent &key,
                          const mozc::commands::Context &context,
                          mozc::commands::Output *output);
  bool IsDirectModeCommand(const mozc::commands::KeyEvent &key) const;
  bool GetConfig(mozc::config::Config *config);
  bool SendCommand(const mozc::commands::SessionCommand &command,
                   mozc::commands::Output *output) {
    return SendCommandWithContext(
        command, mozc::commands::Context::default_instance(), output);
  }
  void set_client_capability(const mozc::commands::Capability &capability);
  bool SyncData();
  bool LaunchTool(const std::string &mode, absl::string_view arg);
  bool LaunchToolWithProtoBuf(const mozc::commands::Output &output);
  // Converts Output message from server to corresponding mozc_tool arguments
  // If launch_tool_mode is not set or NO_TOOL is set or an invalid value is
  // set, this function will return false and do nothing.
  static bool TranslateProtoBufToMozcToolArg(
      const mozc::commands::Output &output, std::string *mode);

 private:
  bool IsValidRunLevel() const {
    return mozc::RunLevel::IsValidClientRunLevel();
  }
  bool EnsureSession();
  bool SendCommandWithContext(const mozc::commands::SessionCommand &command,
                              const mozc::commands::Context &context,
                              mozc::commands::Output *output);
  enum ServerStatus {
    SERVER_INVALID_SESSION,  // current session is not available
    SERVER_OK,               // both server and session are health
  };

  // Initialize input filling id and preferences.
  void InitInput(mozc::commands::Input *input) const;

  bool CreateSession();
  bool DeleteSession();
  bool CallCommand(mozc::commands::Input::CommandType type);

  // This method re-issue session id if it is not available.
  bool EnsureCallCommand(mozc::commands::Input *input,
                         mozc::commands::Output *output);

  // The most primitive Call method
  bool Call(const mozc::commands::Input &input, mozc::commands::Output *output);

  uint64_t id_;
  std::unique_ptr<mozc::commands::Request> request_;
  ServerStatus server_status_;
  // List of key combinations used in the direct input mode.
  std::vector<mozc::KeyInformation> direct_mode_keys_;
  mozc::commands::Capability client_capability_;

  mozc::SessionHandler *session_handler_;
  MozcClientPool *pool_;
  std::string key_;
};
}  // namespace fcitx

#endif
