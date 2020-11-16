/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import React from "react";
import PropTypes from "prop-types";

const specialCharactersRegex = /(?:^|\s)((?:&(?:(?:\w+)|(?:#\d+));)|[_]|[^\w\s])(?:\s*\[([^[\]]+?)\])/g;

function renderSpecialCharacters(description) {
  const matches = Array.from(description.matchAll(specialCharactersRegex));
  if (matches.length == 0) return description;
  let lastIndex = 0;
  const parts = matches.reduce((acc, match, i) => {
    const { index } = match;
    const str = match[0];
    const chars = match[1];
    const meaning = match[2];
    if (index != lastIndex) {
      acc.push(description.substring(lastIndex, index));
    }
    const pos = str.indexOf(chars);
    if (pos > 0) {
      acc.push(description.substr(index, pos));
    }
    acc.push(
      <span key={i} className="symbol" title={meaning ?? ""}>
        <code>{description.substr(index + pos, chars.length)}</code>
        {meaning ? <span className="meaning">{meaning}</span> : undefined}
      </span>
    );
    lastIndex = index + str.length;
    return acc;
  }, []);
  if (lastIndex < description.length) {
    parts.push(description.substr(lastIndex));
  }
  return parts;
}

export default function Description({ description = "" }) {
  return (
    <span className="description">{renderSpecialCharacters(description)}</span>
  );
}

Description.propTypes = {
  description: PropTypes.string.isRequired,
};
