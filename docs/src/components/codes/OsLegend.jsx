/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import operatingSystems from "@site/src/data/operating-systems";

export default function OsLegend() {
  return (
    <div className="codes os legend">
      {operatingSystems.map(({ key, className, heading, title }) => (
        <div key={key} className={"os " + className}>
          <span className="heading">{heading}</span>
          <span className="title">{title}</span>
        </div>
      ))}
    </div>
  );
}

OsLegend.propTypes = {};
