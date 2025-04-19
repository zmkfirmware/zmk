/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import PropTypes from "prop-types";
import { toast } from "react-toastify";
import { CopyToClipboard } from "react-copy-to-clipboard";

export default function ToastyCopyToClipboard({ children, text }) {
  const notify = () =>
    toast(
      <span>
        📋 Copied <code>{text}</code>
      </span>
    );
  return (
    <div onClick={notify}>
      <CopyToClipboard text={text}>{children}</CopyToClipboard>
    </div>
  );
}

ToastyCopyToClipboard.propTypes = {
  children: PropTypes.oneOfType([PropTypes.element, PropTypes.string])
    .isRequired,
  text: PropTypes.string.isRequired,
};
