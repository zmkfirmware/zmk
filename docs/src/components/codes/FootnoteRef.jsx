/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import PropTypes from "prop-types";

export default function FootnoteRef({ children, anchor }) {
  return (
    <a href={"#" + anchor} className="footnoteRef">
      {children}
    </a>
  );
}

FootnoteRef.propTypes = {
  children: PropTypes.oneOfType([PropTypes.string, PropTypes.number])
    .isRequired,
  anchor: PropTypes.string.isRequired,
};
