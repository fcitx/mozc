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

#include "absl/strings/string_view.h"
#include "base/protobuf_util_test.pb.h"
#include "testing/gunit.h"

namespace mozc {
namespace {

TEST(ProtobufUtilTest, SanitizeMessageStrings) {
  protobuf_util::TestMessage style;
  style.set_test_string("test_string");
  style.set_logo_file_name("logo.png");
  style.set_is_enabled(false);

  protobuf_util::TestMessage::TextStyle* text_style =
      style.mutable_shortcut_style();
  text_style->set_font_name("font_name");
  text_style->set_font_size(10);

  protobuf_util::SanitizeMessageStrings(
      style, [](absl::string_view src) -> std::optional<std::string> {
        if (src == "test_string") {
          return "sanitized_test_string";
        }
        if (src == "font_name") {
          return "sanitized_font_name";
        }
        return std::nullopt;  // Unchanged
      });

  EXPECT_EQ(style.test_string(), "sanitized_test_string");  // Changed
  EXPECT_EQ(style.logo_file_name(), "logo.png");            // Unchanged
  EXPECT_FALSE(style.is_enabled());                         // Unchanged
  ASSERT_TRUE(style.has_shortcut_style());
  EXPECT_EQ(style.shortcut_style().font_name(),
            "sanitized_font_name");                    // Changed
  EXPECT_EQ(style.shortcut_style().font_size(), 10);   // Unchanged
}

TEST(ProtobufUtilTest, SanitizeMessageStrings_RepeatedMessage) {
  protobuf_util::TestRepeatedMessage storage;
  protobuf_util::TestRepeatedMessage::Dictionary* dict1 =
      storage.add_dictionaries();
  dict1->set_name("dict1");
  protobuf_util::TestRepeatedMessage::Entry* entry1 = dict1->add_entries();
  entry1->set_key("key1");
  entry1->set_value("value1");
  entry1->set_score(1.5f);

  protobuf_util::TestRepeatedMessage::Dictionary* dict2 =
      storage.add_dictionaries();
  dict2->set_name("dict2");
  protobuf_util::TestRepeatedMessage::Entry* entry2 = dict2->add_entries();
  entry2->set_key("key2");
  entry2->set_value("value2");

  protobuf_util::SanitizeMessageStrings(
      storage, [](absl::string_view src) -> std::optional<std::string> {
        if (src == "dict1") {
          return "sanitized_dict1";
        }
        if (src == "key2") {
          return "sanitized_key2";
        }
        if (src == "value1") {
          return "sanitized_value1";
        }
        return std::nullopt;
      });

  EXPECT_EQ(storage.dictionaries(0).name(), "sanitized_dict1");
  EXPECT_EQ(storage.dictionaries(0).entries(0).key(), "key1");
  EXPECT_EQ(storage.dictionaries(0).entries(0).value(), "sanitized_value1");
  EXPECT_EQ(storage.dictionaries(0).entries(0).score(), 1.5f);  // Unchanged

  EXPECT_EQ(storage.dictionaries(1).name(), "dict2");
  EXPECT_EQ(storage.dictionaries(1).entries(0).key(), "sanitized_key2");
  EXPECT_EQ(storage.dictionaries(1).entries(0).value(), "value2");
}

}  // namespace
}  // namespace mozc
