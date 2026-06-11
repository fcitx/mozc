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

#include "base/protobuf_util.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/functional/function_ref.h"
#include "absl/strings/string_view.h"
#include "base/protobuf/descriptor.h"
#include "base/protobuf/message.h"

namespace mozc {
namespace protobuf_util {

void SanitizeMessageStrings(
    protobuf::Message& message,
    absl::FunctionRef<std::optional<std::string>(absl::string_view)>
        sanitizer) {
  const protobuf::Reflection* reflection = message.GetReflection();

  // ListFields only populates fields that are explicitly set in the message.
  std::vector<const protobuf::FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);

  for (const protobuf::FieldDescriptor* field : fields) {
    // Recursively sanitize nested messages.
    if (field->type() == protobuf::FieldDescriptor::TYPE_MESSAGE) {
      if (field->is_repeated()) {
        const int count = reflection->FieldSize(message, field);
        for (int i = 0; i < count; ++i) {
          protobuf::Message* sub_message =
              reflection->MutableRepeatedMessage(&message, field, i);
          SanitizeMessageStrings(*sub_message, sanitizer);
        }
      } else {
        protobuf::Message* sub_message =
            reflection->MutableMessage(&message, field);
        SanitizeMessageStrings(*sub_message, sanitizer);
      }
      continue;
    }

    // Only sanitize strings.
    if (field->type() != protobuf::FieldDescriptor::TYPE_STRING) {
      continue;
    }

    // Sanitize a non-repeated string.
    if (!field->is_repeated()) {
      protobuf::Reflection::ScratchSpace scratch;
      const absl::string_view val =
          reflection->GetStringView(message, field, scratch);
      if (std::optional<std::string> new_val = sanitizer(val);
          new_val.has_value()) {
        reflection->SetString(&message, field, *std::move(new_val));
      }
      continue;
    }

    // Sanitize repeated strings.
    const int count = reflection->FieldSize(message, field);
    protobuf::Reflection::ScratchSpace scratch;
    for (int i = 0; i < count; ++i) {
      const absl::string_view val =
          reflection->GetRepeatedStringView(message, field, i, scratch);
      if (std::optional<std::string> new_val = sanitizer(val);
          new_val.has_value()) {
        reflection->SetRepeatedString(&message, field, i, *std::move(new_val));
      }
    }
  }
}

}  // namespace protobuf_util
}  // namespace mozc
