# -*- coding: utf-8 -*-
# Copyright 2010-2021, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Generate a C++ array definition for file embedding."""
import argparse
import os
import sys
import textwrap


def _ParseArgs():
  """Parse command line options."""
  parser = argparse.ArgumentParser()
  parser.add_argument('--input')
  parser.add_argument('--name')
  parser.add_argument(
      '--output', type=argparse.FileType(mode='w', encoding='utf-8')
  )
  parser.add_argument('--as_string_view', action='store_true')
  parser.add_argument(
      '--skip_until',
      help='If specified, drops all lines in the file before the first line '
      'containing this string. The line containing the string is kept.',
  )
  return parser.parse_args()


def _EmbedAsStringView(input_path, name, output_file, skip_until=None):
  """Embed file as absl::string_view using Raw String Literal.

  Args:
    input_path: Path to the input file to be embedded.
    name: The name of the `absl::string_view` variable to be generated.
    output_file: The file object where the generated C++ code will be written.
    skip_until: If specified, lines in the input file before the first line
      containing this string are skipped. The line containing the string is
      included in the output.
  """
  # Load input file as binary to validate UTF-8 and Null bytes
  with open(input_path, 'rb') as infile:
    data = infile.read()

  # Validate invalid UTF-8 byte sequence
  try:
    content = data.decode('utf-8', errors='strict')
  except UnicodeDecodeError as e:
    sys.stderr.write(
        f'Error: {input_path} contains invalid UTF-8 byte sequence: {e}\n'
    )
    sys.exit(1)

  # Skip lines up to the first occurrence of `skip_until`.
  # This is useful for stripping off license headers or other text inserted by
  # Copybara and keeping only the core contents (e.g. from `# proto-file:`).
  if skip_until:
    lines = content.splitlines(keepends=True)
    for i, line in enumerate(lines):
      if skip_until in line:
        content = ''.join(lines[i:])
        break

  # Validate Null byte
  if '\0' in content:
    sys.stderr.write(
        f'Error: {input_path} contains a null byte (\\0), which is not'
        ' allowed in as_string_view mode.\n'
    )
    sys.exit(1)

  # Validate delimiter conflict
  # C++ Raw String literal delimiters have a length limit of 16 characters.
  delimiter = name[:16]
  delimiter_conflict = f'){delimiter}'
  if delimiter_conflict in content:
    sys.stderr.write(
        f'Error: {input_path} contains the delimiter string'
        f" '{delimiter_conflict}', which conflicts with the C++ Raw String"
        ' delimiter.\n'
    )
    sys.exit(1)

  output_file.write(textwrap.dedent(f"""\
      #ifdef MOZC_EMBEDDED_FILE_{name}
      #error "{name} was already included or defined elsewhere"
      #else
      #define MOZC_EMBEDDED_FILE_{name}
      constexpr absl::string_view {name} = R"{delimiter}({content}){delimiter}";
      #endif  // MOZC_EMBEDDED_FILE_{name}
      """))


def _EmbedAsUint64Array(input_path, name, output_file):
  """Embed file as uint64_t array using EmbeddedFile struct."""

  output_file.write(textwrap.dedent(f"""\
      #ifdef MOZC_EMBEDDED_FILE_{name}
      #error "{name} was already included or defined elsewhere"
      #else
      #define MOZC_EMBEDDED_FILE_{name}
      constexpr uint64_t {name}_data[] = {{
      """))

  with open(input_path, 'rb') as infile:
    # Read in chunks of 32 bytes (representing one printed line of 4 uint64_t
    # values)
    while line_chunk := infile.read(32):
      # Split the 32-byte row into 8-byte chunks
      hex_strings = []
      for i in range(0, len(line_chunk), 8):
        chunk_bytes = line_chunk[i : i + 8]
        chunk_int = int.from_bytes(chunk_bytes, byteorder='little')
        # Format C++ hexadecimal string literals for each chunk
        hex_strings.append(f'0x{chunk_int:016x}')
      # Write the entire row to the output file (indenting with 2 spaces)
      output_file.write('  ' + ', '.join(hex_strings) + ',\n')

  size = os.stat(input_path).st_size
  output_file.write(textwrap.dedent(f"""\
      }};
      constexpr EmbeddedFile {name} = {{
        {name}_data,
        {size},
      }};
      #endif  // MOZC_EMBEDDED_FILE_{name}
      """))


def main():
  args = _ParseArgs()
  if args.as_string_view:
    _EmbedAsStringView(args.input, args.name, args.output, args.skip_until)
  else:
    if args.skip_until:
      sys.stderr.write(
          'Error: --skip_until is only supported when --as_string_view is '
          'enabled.\n'
      )
      sys.exit(1)
    _EmbedAsUint64Array(args.input, args.name, args.output)


if __name__ == '__main__':
  main()
