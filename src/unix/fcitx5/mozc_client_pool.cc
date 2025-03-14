// Copyright 2023-2023, Weng Xuetian <wengxt@gmail.com>
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

#include "unix/fcitx5/mozc_client_pool.h"

#include <fcitx-utils/charutils.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include "protocol/commands.pb.h"
#include "unix/fcitx5/mozc_client_interface.h"

namespace fcitx {

MozcClientPool::MozcClientPool(PropertyPropagatePolicy initialPolicy)
    : policy_(initialPolicy) {}

void MozcClientPool::setPolicy(PropertyPropagatePolicy policy) {
  if (policy_ == policy) {
    return;
  }

  assert(clients_.empty());
  policy_ = policy;
}

std::string uuidKey(InputContext *ic) {
  std::string key = "u:";
  for (auto v : ic->uuid()) {
    auto lower = v % 16;
    auto upper = v / 16;
    key.push_back(charutils::toHex(upper));
    key.push_back(charutils::toHex(lower));
  }
  return key;
}

std::shared_ptr<MozcClientInterface> MozcClientPool::requestClient(
    InputContext *ic) {
  std::string key;
  switch (policy_) {
    case PropertyPropagatePolicy::No:
      key = uuidKey(ic);
      break;
    case PropertyPropagatePolicy::Program:
      if (!ic->program().empty()) {
        key = stringutils::concat("p:", ic->program());
      } else {
        key = uuidKey(ic);
      }
      break;
    case PropertyPropagatePolicy::All:
      key = "g:";
      break;
  }
  auto iter = clients_.find(key);
  if (iter != clients_.end()) {
    return iter->second.lock();
  }
  std::unique_ptr<MozcClientInterface> newclient = createClient();
  // Currently client capability is fixed.
  mozc::commands::Capability capability;
  capability.set_text_deletion(
      mozc::commands::Capability::DELETE_PRECEDING_TEXT);
  newclient->set_client_capability(capability);
  return registerClient(key, std::move(newclient));
}

std::shared_ptr<MozcClientInterface> MozcClientPool::registerClient(
    const std::string &key, std::unique_ptr<MozcClientInterface> client) {
  assert(!key.empty());
  std::shared_ptr<MozcClientInterface> managedClient(
      client.release(),
      [key, ref = this->watch()](MozcClientInterface *client) {
        if (auto *that = ref.get()) {
          that->unregisterClient(key);
        }
        delete client;
      });
  const auto [iter, success] = clients_.emplace(key, managedClient);
  FCITX_UNUSED(success);
  assert(success);
  return managedClient;
}

void MozcClientPool::unregisterClient(const std::string &key) {
  clients_.erase(key);
}

}  // namespace fcitx
