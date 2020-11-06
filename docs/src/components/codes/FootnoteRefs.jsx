/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import React from "react";
import PropTypes from "prop-types";
import FootnoteRef from "./FootnoteRef";

function joinReactElements(arr, delimiter) {
  return arr.reduce((acc, fragment) => {
    if (acc === null) {
      return fragment;
    }
    return (
      <>
        {acc}
        {delimiter}
        {fragment}
      </>
    );
  }, null);
}

export default function FootnoteRefs({ footnotes }) {
  return (
    <span className="footnoteRefs">
      {joinReactElements(
        footnotes.map((footnote) => (
          <FootnoteRef key={footnote.reference} {...footnote}>
            {footnote.symbol}
          </FootnoteRef>
        )),
        ", "
      )}
    </span>
  );
}

FootnoteRefs.propTypes = {
  footnotes: PropTypes.array.isRequired,
};
