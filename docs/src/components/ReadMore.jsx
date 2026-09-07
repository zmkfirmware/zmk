/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import { faBook } from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import classNames from "classnames";
import PropTypes from "prop-types";

import styles from "./ReadMore.module.css";

export default function ReadMore({ children, className }) {
  return (
    <aside className={classNames(styles.readMore, className)}>
      <span className={styles.icon} aria-hidden="true">
        <FontAwesomeIcon icon={faBook} />
      </span>
      <div className={styles.content}>{children}</div>
    </aside>
  );
}

ReadMore.propTypes = {
  children: PropTypes.node.isRequired,
  className: PropTypes.string,
};

ReadMore.defaultProps = {
  className: undefined,
};
