// Copyright 2010-2021, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// session_handler_main.cc
//
// Usage:
// session_handler_main --input input.txt --profile /tmp/mozc
//                      --dictionary oss --engine desktop
//
// session_handler_main --test --input input.txt --profile /tmp/mozc
//
/* Example of input.txt (tsv format)
# Enable IME
SEND_KEY        ON

SET_MOBILE_REQUEST

RESET_CONTEXT
UPDATE_MOBILE_KEYBOARD  QWERTY_MOBILE_TO_HIRAGANA        COMMIT
SWITCH_INPUT_MODE       HIRAGANA

SEND_KEYS       arigatou
SEND_KEY        Enter
# EXPAND_SUGGESTION
# SHOW_OUTPUT
SHOW
SHOW_LOG_BY_VALUE       ございます
SHOW_LOG_BY_VALUE       ございました
*/

#include <cstdint>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "base/file_stream.h"
#include "base/init_mozc.h"
#include "base/system_util.h"
#include "data_manager/oss/oss_data_manager.h"
#include "data_manager/testing/mock_data_manager.h"
#include "engine/engine.h"
#include "protocol/candidate_window.pb.h"
#include "protocol/commands.pb.h"
#include "session/session_handler_tool.h"

ABSL_DECLARE_FLAG(bool, use_history_rewriter);

ABSL_FLAG(std::string, input, "", "Input file. ");
ABSL_FLAG(std::string, profile, "", "User profile directory");
ABSL_FLAG(std::string, engine, "", "Conversion engine: 'mobile' or 'desktop'");
ABSL_FLAG(std::string, dictionary, "", "Dictionary: 'oss' or 'test'");
ABSL_FLAG(bool, test, false, "Run as a test and quit.");

namespace mozc {
void Show(const commands::Output &output) {
  for (const auto &segment : output.preedit().segment()) {
    std::cout << segment.value() << " ";
  }
  std::cout << "(" << output.preedit().cursor() << ")" << std::endl;
  for (const auto &candidate : output.candidate_window().candidate()) {
    std::cout << candidate.id() << ": " << candidate.value() << std::endl;
  }
}

void ShowLog(const commands::Output &output, const int cand_id) {
  for (const auto &candidate : output.all_candidate_words().candidates()) {
    if (candidate.id() == cand_id) {
      std::cout << candidate.value() << std::endl;
      std::cout << candidate.log() << std::endl;
      return;
    }
  }

  for (const auto &candidate :
       output.removed_candidate_words_for_debug().candidates()) {
    if (candidate.id() == cand_id) {
      std::cout << candidate.value() << std::endl;
      std::cout << candidate.log() << std::endl;
      return;
    }
  }
}

bool ParseLine(session::SessionHandlerInterpreter &handler, std::string line,
               int line_number) {
  std::vector<std::string> args = handler.Parse(line);
  if (args.empty()) {
    return true;
  }
  const std::string &command = args[0];

  if (command == "SHOW_ALL") {
    std::cout << absl::StrCat(handler.LastOutput()) << std::endl;
    return true;
  }
  if (command == "SHOW_OUTPUT") {
    commands::Output output = handler.LastOutput();
    output.mutable_removed_candidate_words_for_debug()->Clear();
    std::cout << absl::StrCat(output.Utf8DebugString()) << std::endl;
    return true;
  }
  if (command == "SHOW_RESULT") {
    const commands::Output &output = handler.LastOutput();
    std::cout << absl::StrCat(output.result().Utf8DebugString()) << std::endl;
    return true;
  }
  if (command == "SHOW_CANDIDATES") {
    std::cout << absl::StrCat(
                     handler.LastOutput().candidate_window().Utf8DebugString())
              << std::endl;
    return true;
  }
  if (command == "SHOW_REMOVED_CANDIDATES") {
    std::cout << absl::StrCat(handler.LastOutput()
                                  .removed_candidate_words_for_debug()
                                  .Utf8DebugString())
              << std::endl;
    return true;
  }
  if (command == "SHOW") {
    Show(handler.LastOutput());
    return true;
  }
  if (command == "SHOW_LOG") {
    uint32_t id;
    if (args.size() == 2 && absl::SimpleAtoi(args[1], &id)) {
      ShowLog(handler.LastOutput(), id);
    } else {
      std::cout << "#" << line_number << ": " << line << std::endl;
      std::cout << "ERROR: syntax error" << std::endl;
    }
    return false;
  }
  if (command == "SHOW_LOG_BY_VALUE") {
    if (args.size() != 2) {
      std::cout << "#" << line_number << ": " << line << std::endl;
      std::cout << "ERROR: syntax error" << std::endl;
      return false;
    }
    for (const uint32_t id : handler.GetCandidateIdsByValue(args[1])) {
      ShowLog(handler.LastOutput(), id);
    }
    for (const uint32_t id : handler.GetRemovedCandidateIdsByValue(args[1])) {
      ShowLog(handler.LastOutput(), id);
    }
    return true;
  }

  const absl::Status status = handler.Eval(args);
  if (!status.ok()) {
    std::cout << "#" << line_number << ": " << line << std::endl;
    std::cout << "ERROR: " << status.message() << std::endl;
    return false;
  }
  return true;
}

std::unique_ptr<const DataManager> CreateDataManager(
    const std::string &dictionary) {
  if (dictionary == "oss") {
    return std::make_unique<const oss::OssDataManager>();
  }
  if (dictionary == "mock") {
    return std::make_unique<const testing::MockDataManager>();
  }
  if (!dictionary.empty()) {
    std::cout << "ERROR: Unknown dictionary name: " << dictionary << std::endl;
  }

  return std::make_unique<const oss::OssDataManager>();
}

absl::StatusOr<std::unique_ptr<Engine>> CreateEngine(
    const std::string &engine, const std::string &dictionary) {
  if (engine == "desktop") {
    return Engine::CreateDesktopEngine(CreateDataManager(dictionary));
  }
  if (engine == "mobile") {
    return Engine::CreateMobileEngine(CreateDataManager(dictionary));
  }
  if (!engine.empty()) {
    std::cout << "ERROR: Unknown engine name: " << engine << std::endl;
  }
  return Engine::CreateMobileEngine(CreateDataManager(dictionary));
}

}  // namespace mozc

int main(int argc, char **argv) {
  mozc::InitMozc(argv[0], &argc, &argv);
  if (!absl::GetFlag(FLAGS_profile).empty()) {
    mozc::SystemUtil::SetUserProfileDirectory(absl::GetFlag(FLAGS_profile));
  }

  std::string engine_name = absl::GetFlag(FLAGS_engine);
  std::string dictionary_name = absl::GetFlag(FLAGS_dictionary);
  const bool is_test = absl::GetFlag(FLAGS_test);
  if (is_test) {
    absl::SetFlag(&FLAGS_use_history_rewriter, true);
    if (engine_name.empty()) {
      engine_name = "desktop";
    }
    if (dictionary_name.empty()) {
      dictionary_name = "mock";
    }
  }

  auto engine = mozc::CreateEngine(engine_name, dictionary_name);
  if (!engine.ok()) {
    std::cout << "engine init error" << std::endl;
    return 1;
  }
  mozc::session::SessionHandlerInterpreter handler(*std::move(engine));

  std::string line;
  int line_number = 1;
  if (const std::string filepath = absl::GetFlag(FLAGS_input);
      !filepath.empty()) {
    mozc::InputFileStream input(filepath);
    bool is_passed = true;
    while (std::getline(input, line)) {
      is_passed &= mozc::ParseLine(handler, line, line_number++);
    }

    if (is_test) {
      if (is_passed) {
        std::cout << "[ PASSED ] " << filepath << std::endl;
        return 0;
      } else {
        std::cout << "[ FAILED ] " << filepath << std::endl;
        return 1;
      }
    }
  }

  while (std::getline(std::cin, line)) {
    mozc::ParseLine(handler, line, line_number++);
  }
  return 0;
}
