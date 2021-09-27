/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

import React from "react";
import PropTypes from "prop-types";

function CustomBoardForm({
  bindPsuType,
  bindOutputV,
  bindEfficiency,
  bindQuiescentMicroA,
  bindOtherQuiescentMicroA,
}) {
  return (
    <div className="profilerSection">
      <h3>Custom Board</h3>
      <div className="row">
        <div className="col col--4">
          <div className="profilerInput">
            <label>Power Supply Type</label>
            <select {...bindPsuType}>
              <option hidden value="">
                Select a PSU type
              </option>
              <option value="LDO">LDO</option>
              <option value="SWITCHING">Switching</option>
            </select>
          </div>
        </div>
        <div className="col col--4">
          <div className="profilerInput">
            <label>
              Output Voltage{" "}
              <span tooltip="Output Voltage of the PSU used by the system">
                ⓘ
              </span>
            </label>
            <input {...bindOutputV} type="range" min="1.8" step=".1" max="5" />
            <span>{parseFloat(bindOutputV.value).toFixed(1)}V</span>
          </div>
          {bindPsuType.value === "SWITCHING" && (
            <div className="profilerInput">
              <label>
                PSU Efficiency{" "}
                <span tooltip="The estimated efficiency with a VIN of 3.8 and the output voltage entered above">
                  ⓘ
                </span>
              </label>
              <input
                {...bindEfficiency}
                type="range"
                min=".50"
                step=".01"
                max="1"
              />
              <span>{Math.round(bindEfficiency.value * 100)}%</span>
            </div>
          )}
        </div>
        <div className="col col--4">
          <div className="profilerInput">
            <label>
              PSU Quiescent{" "}
              <span tooltip="The standby usage of the PSU">ⓘ</span>
            </label>
            <div className="inputBox">
              <input {...bindQuiescentMicroA} type="number" />
              <span>µA</span>
            </div>
          </div>
          <div className="profilerInput">
            <label>
              Other Quiescent{" "}
              <span tooltip="Any other standby usage of the board (voltage dividers, extra ICs, etc)">
                ⓘ
              </span>
            </label>
            <div className="inputBox">
              <input {...bindOtherQuiescentMicroA} type="number" />
              <span>µA</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

CustomBoardForm.propTypes = {
  bindPsuType: PropTypes.Object,
  bindOutputV: PropTypes.Object,
  bindEfficiency: PropTypes.Object,
  bindQuiescentMicroA: PropTypes.Object,
  bindOtherQuiescentMicroA: PropTypes.Object,
};

export default CustomBoardForm;
