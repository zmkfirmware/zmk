/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import PropTypes from "prop-types";
import Footnote from "./Footnote";

export default function Footnotes({ footnotes = [], id }) {
  return (
    <div className="footnotes">
      <a id={id} className="anchor" />
      <div className="label">Notes</div>
      <div className="notes">
        {footnotes.map((footnote) => (
          <Footnote key={footnote.id} {...footnote}>
            {footnote.value}
          </Footnote>
        ))}
      </div>
    </div>
  );
}

Footnotes.propTypes = {
  footnotes: PropTypes.array.isRequired,
  id: PropTypes.string.isRequired,
};
