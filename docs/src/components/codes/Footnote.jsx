/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import PropTypes from "prop-types";

export default function Footnote({ children, symbol, id }) {
  return (
    <div className="footnote" id={id}>
      <div className="symbol">{symbol}</div>
      <div className="content">{children}</div>
    </div>
  );
}

Footnote.propTypes = {
  children: PropTypes.element.isRequired,
  symbol: PropTypes.oneOfType([PropTypes.string, PropTypes.number]).isRequired,
  id: PropTypes.string.isRequired,
};
