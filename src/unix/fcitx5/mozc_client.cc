#include "base/vlog.h"
#include "session/key_info_util.h"
#include "unix/fcitx5/mozc_client_pool.h"

#ifdef _WIN32
#include <windows.h>

#include "base/win32/wide_char.h"
#include "base/win32/win_util.h"
#else  // _WIN32
#include <unistd.h>
#endif  // _WIN32

namespace fcitx {

MozcClient::MozcClient(mozc::SessionHandler *session_handler)
    : id_(0),
      session_handler_(session_handler),
      server_status_(SERVER_INVALID_SESSION) {
  // Initialize direct_mode_keys_
  mozc::config::Config config;
  mozc::config::ConfigHandler::GetConfig(&config);
  direct_mode_keys_ = mozc::KeyInfoUtil::ExtractSortedDirectModeKeys(config);
  InitRequestForSvsJapanese(true);
}

MozcClient::~MozcClient() {
  if (pool_) {
    pool_->unregisterClient(key_);
  }
  DeleteSession();
}

void MozcClient::InitRequestForSvsJapanese(bool use_svs) {
  request_ = std::make_unique<mozc::commands::Request>();

  mozc::commands::DecoderExperimentParams params;
  uint32_t variation_types = params.variation_character_types();
  if (use_svs) {
    variation_types |= mozc::commands::DecoderExperimentParams::SVS_JAPANESE;
  } else {
    variation_types &= ~mozc::commands::DecoderExperimentParams::SVS_JAPANESE;
  }
  request_->mutable_decoder_experiment_params()->set_variation_character_types(
      variation_types);
}

bool MozcClient::EnsureSession() {
  if (server_status_ == SERVER_OK) {
    return true;
  }
  if (!CreateSession()) {
    LOG(ERROR) << "CreateSession failed";
    return false;
  }

  // Call SET_REQUEST if request_ is not nullptr.
  if (request_) {
    mozc::commands::Input input;
    input.set_id(id_);
    input.set_type(mozc::commands::Input::SET_REQUEST);
    *input.mutable_request() = *request_;
    mozc::commands::Output output;
    Call(input, &output);
  }

  server_status_ = SERVER_OK;
  return true;
}

bool MozcClient::SendKeyWithContext(const mozc::commands::KeyEvent &key,
                                    const mozc::commands::Context &context,
                                    mozc::commands::Output *output) {
  mozc::commands::Input input;
  input.set_type(mozc::commands::Input::SEND_KEY);
  *input.mutable_key() = key;
  // If the pointer of |context| is not the default_instance, update the data.
  if (&context != &mozc::commands::Context::default_instance()) {
    *input.mutable_context() = context;
  }
  return EnsureCallCommand(&input, output);
}

bool MozcClient::SendCommandWithContext(
    const mozc::commands::SessionCommand &command,
    const mozc::commands::Context &context, mozc::commands::Output *output) {
  mozc::commands::Input input;
  input.set_type(mozc::commands::Input::SEND_COMMAND);
  *input.mutable_command() = command;
  // If the pointer of |context| is not the default_instance, update the data.
  if (&context != &mozc::commands::Context::default_instance()) {
    *input.mutable_context() = context;
  }
  return EnsureCallCommand(&input, output);
}

bool MozcClient::EnsureCallCommand(mozc::commands::Input *input,
                                   mozc::commands::Output *output) {
  if (!EnsureSession()) {
    LOG(ERROR) << "EnsureSession failed";
    return false;
  }
  InitInput(input);
  output->set_id(0);

  if (!Call(*input, output)) {
    LOG(ERROR) << "Call command failed";
    return false;
  }
  return true;
}

void MozcClient::set_client_capability(
    const mozc::commands::Capability &capability) {
  client_capability_ = capability;
}

bool MozcClient::CreateSession() {
  id_ = 0;
  mozc::commands::Input input;
  input.set_type(mozc::commands::Input::CREATE_SESSION);

  *input.mutable_capability() = client_capability_;

  mozc::commands::Output output;
  if (!Call(input, &output)) {
    return false;
  }

  if (output.error_code() != mozc::commands::Output::SESSION_SUCCESS) {
    LOG(ERROR) << "Server returns an error";
    server_status_ = SERVER_INVALID_SESSION;
    return false;
  }

  id_ = output.id();
  return true;
}

bool MozcClient::DeleteSession() {
  // No need to delete session
  if (id_ == 0) {
    return true;
  }

  mozc::commands::Input input;
  InitInput(&input);
  input.set_type(mozc::commands::Input::DELETE_SESSION);

  mozc::commands::Output output;
  if (!Call(input, &output)) {
    LOG(ERROR) << "DeleteSession failed";
    return false;
  }
  id_ = 0;
  return true;
}

bool MozcClient::IsDirectModeCommand(
    const mozc::commands::KeyEvent &key) const {
  return mozc::KeyInfoUtil::ContainsKey(direct_mode_keys_, key);
}

bool MozcClient::GetConfig(mozc::config::Config *config) {
  mozc::commands::Input input;
  InitInput(&input);
  input.set_type(mozc::commands::Input::GET_CONFIG);

  mozc::commands::Output output;
  if (!Call(input, &output)) {
    return false;
  }

  if (!output.has_config()) {
    return false;
  }

  config->Clear();
  *config = output.config();
  return true;
}

bool MozcClient::SyncData() {
  return CallCommand(mozc::commands::Input::SYNC_DATA);
}

bool MozcClient::CallCommand(mozc::commands::Input::CommandType type) {
  mozc::commands::Input input;
  InitInput(&input);
  input.set_type(type);
  mozc::commands::Output output;
  return Call(input, &output);
}

bool MozcClient::Call(const mozc::commands::Input &input,
                      mozc::commands::Output *output) {
  mozc::commands::Command command;
  *command.mutable_input() = input;
  if (!session_handler_->EvalCommand(&command)) {
    return false;
  }
  *output = command.output();
  return true;
}

void MozcClient::InitInput(mozc::commands::Input *input) const {
  input->set_id(id_);
}

bool MozcClient::TranslateProtoBufToMozcToolArg(
    const mozc::commands::Output &output, std::string *mode) {
  if (!output.has_launch_tool_mode() || mode == nullptr) {
    return false;
  }

  switch (output.launch_tool_mode()) {
    case mozc::commands::Output::CONFIG_DIALOG:
      mode->assign("config_dialog");
      break;
    case mozc::commands::Output::DICTIONARY_TOOL:
      mode->assign("dictionary_tool");
      break;
    case mozc::commands::Output::WORD_REGISTER_DIALOG:
      mode->assign("word_register_dialog");
      break;
    case mozc::commands::Output::NO_TOOL:
    default:
      // do nothing
      return false;
      break;
  }

  return true;
}

bool MozcClient::LaunchToolWithProtoBuf(const mozc::commands::Output &output) {
  std::string mode;
  if (!TranslateProtoBufToMozcToolArg(output, &mode)) {
    return false;
  }

  // TODO(nona): extends output message to support extra argument.
  return LaunchTool(mode, "");
}

bool MozcClient::LaunchTool(const std::string &mode,
                            const absl::string_view extra_arg) {
  // Don't execute any child process if the parent process is not
  // in proper runlevel.
  if (!IsValidRunLevel()) {
    return false;
  }

  constexpr size_t kModeMaxSize = 32;
  if (mode.empty() || mode.size() >= kModeMaxSize) {
    LOG(ERROR) << "Invalid mode: " << mode;
    return false;
  }

  if (mode == "administration_dialog") {
    return false;
  }

#if defined(_WIN32) || defined(__linux__)
  std::string arg = absl::StrCat("--mode=", mode);
  if (!extra_arg.empty()) {
    absl::StrAppend(&arg, " ", extra_arg);
  }
  if (!mozc::Process::SpawnMozcProcess(kMozcTool, arg)) {
    LOG(ERROR) << "Cannot execute: " << kMozcTool << " " << arg;
    return false;
  }
#endif  // _WIN32 || __linux__

  return true;
}
}  // namespace fcitx
