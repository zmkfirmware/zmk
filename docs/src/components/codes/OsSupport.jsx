/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import PropTypes from "prop-types";
import OsSupportIcon from "./OsSupportIcon";
import FootnoteRefs from "./FootnoteRefs";

export default function OsSupport({ value, footnotes = [] }) {
  return (
    <>
      <OsSupportIcon value={value} />
      {footnotes.length > 0 ? (
        <FootnoteRefs footnotes={footnotes} />
      ) : undefined}
    </>
  );
}

OsSupport.propTypes = {
  value: PropTypes.oneOf([true, false, null]),
  footnotes: PropTypes.array.isRequired,
};
