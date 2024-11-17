#include "unix/fcitx5/mozc_client_interface.h"

#include "unix/fcitx5/mozc_client_pool.h"

namespace fcitx {
MozcClientInterface::~MozcClientInterface() {
  if (pool_) {
    pool_->unregisterClient(key_);
  }
}
}  // namespace fcitx
