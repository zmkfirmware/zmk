/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import PropTypes from "prop-types";

const Icon = ({ children, className, title }) => (
  <span className={className} title={title}>
    {children}
  </span>
);

Icon.propTypes = {
  children: PropTypes.oneOfType([PropTypes.element, PropTypes.string])
    .isRequired,
  className: PropTypes.string.isRequired,
  title: PropTypes.string.isRequired,
};

export const Supported = () => (
  <Icon className="supported" title="Supported 😄">
    ⭐
  </Icon>
);
export const NotSupported = () => (
  <Icon className="not-supported" title="Not Supported 😢">
    ❌
  </Icon>
);
export const NotTested = () => (
  <Icon className="not-tested" title="Not Tested Yet - PR's welcomed! 🧐">
    ❔
  </Icon>
);

export default function OsSupportIcon({ value }) {
  if (value === true) {
    return <Supported />;
  }
  if (value === false) {
    return <NotSupported />;
  }
  return <NotTested />;
}

OsSupportIcon.propTypes = {
  value: PropTypes.oneOf([true, false, null]),
};
