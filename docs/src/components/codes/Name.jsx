/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import React from "react";
import PropTypes from "prop-types";
import ToastyCopyToClipboard from "./ToastyCopyToClipboard";

export default function Name({ children, name }) {
  return (
    <ToastyCopyToClipboard text={name}>
      <code className="name" title="Copy 📋">
        {children}
      </code>
    </ToastyCopyToClipboard>
  );
}

Name.propTypes = {
  children: PropTypes.oneOfType([PropTypes.element, PropTypes.string])
    .isRequired,
  name: PropTypes.string.isRequired,
};
