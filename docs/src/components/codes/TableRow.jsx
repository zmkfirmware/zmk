/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import PropTypes from "prop-types";
import Name from "./Name";
import Description from "./Description";
import Context from "./Context";
import LinkIcon from "./LinkIcon";
import OsSupport from "./OsSupport";
import operatingSystems from "@site/src/data/operating-systems";

export default function TableRow({
  names,
  description,
  context = "",
  clarify = false,
  documentation,
  os,
  footnotes,
  tableFootnotes,
}) {
  return (
    <tr>
      <td className="names">
        {names.map((name) => (
          <Name key={name} name={name}>
            {name}
          </Name>
        ))}
      </td>
      <td className="description">
        <Description description={description} />
        {clarify && context ? <Context>{context}</Context> : undefined}
      </td>
      <td className="documentation" title="Documentation">
        <a href={documentation} target="_blank" rel="noreferrer">
          <LinkIcon />
        </a>
      </td>
      {operatingSystems.map(({ key, className, title }) => (
        <td key={key} className={`os ${className}`} title={title}>
          <OsSupport
            value={os[key]}
            footnotes={tableFootnotes.filter(
              ({ id }) =>
                (Array.isArray(footnotes[key]) &&
                  footnotes[key].includes(id)) ||
                footnotes[key] == id
            )}
          />
        </td>
      ))}
    </tr>
  );
}

TableRow.propTypes = {
  names: PropTypes.array.isRequired,
  description: PropTypes.string.isRequired,
  context: PropTypes.string.isRequired,
  clarify: PropTypes.bool,
  documentation: PropTypes.string.isRequired,
  os: PropTypes.object.isRequired,
  footnotes: PropTypes.object.isRequired,
  tableFootnotes: PropTypes.array.isRequired,
};
