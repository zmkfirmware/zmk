/*
  Converted from https://github.com/highlightjs/highlight.js/blob/main/src/styles/github.css

  BSD 3-Clause License

  Copyright (c) 2006, Ivan Sagalaev.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the name of the copyright holder nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/** @type {import("prism-react-renderer").PrismTheme} */
const theme = {
  plain: { color: "#24292e", backgroundColor: "#f9f9f9" },
  styles: [
    { types: ["keyword", "atrule"], style: { color: "#d73a49" } },
    { types: ["class-name", "function"], style: { color: "#6f42c1" } },
    {
      types: [
        "attr-name",
        "boolean",
        "important",
        "doctype",
        "prolog",
        "cdata",
        "number",
        "operator",
        "variable",
        "selector",
      ],
      style: { color: "#005cc5" },
    },
    { types: ["regex", "string", "char", "url"], style: { color: "#032f62" } },
    { types: ["builtin", "symbol", "entity"], style: { color: "#e36209" } },
    { types: ["comment"], style: { color: "#6a737d" } },
    { types: ["italic"], style: { color: "#24292e", fontStyle: "italic" } },
    { types: ["bold"], style: { color: "#24292e", fontWeight: "bold" } },
    {
      types: ["inserted"],
      style: { color: "#22863a", backgroundColor: "#f0fff4" },
    },
    {
      types: ["deleted"],
      style: { color: "#b31d28", backgroundColor: "#ffeef0" },
    },
    { types: ["property", "punctuation", "tag"], style: {} },
  ],
};

module.exports = theme;
