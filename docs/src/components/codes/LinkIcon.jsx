/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";
import { faExternalLinkAlt } from "@fortawesome/free-solid-svg-icons";
export default function LinkIcon() {
  return <FontAwesomeIcon className="icon" icon={faExternalLinkAlt} />;
}

LinkIcon.propTypes = {};
