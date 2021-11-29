/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

import React, { useState } from "react";
import classnames from "classnames";
import Layout from "@theme/Layout";
import styles from "./styles.module.css";
import PowerEstimate from "../components/power-estimate";
import CustomBoardForm from "../components/custom-board-form";
import { useInput } from "../utils/hooks";
import { zmkBoards } from "../data/power";
import "../css/power-profiler.css";

const Disclaimer = `This profiler makes many assumptions about typing
              activity, battery characteristics, hardware behavior, and
              doesn't account for error of user inputs. For example battery
              mAh, which is often incorrectly advertised higher than it's actual capacity.
              While it tries to estimate power usage using real power readings of ZMK,
              every person will have different results that may be worse or even
              better than the estimation given here.`;

function PowerProfiler() {
  const { value: board, bind: bindBoard } = useInput("");
  const { value: split, bind: bindSplit } = useInput(false);
  const { value: batteryMilliAh, bind: bindBatteryMilliAh } = useInput(110);

  const { value: psuType, bind: bindPsuType } = useInput("");
  const { value: outputV, bind: bindOutputV } = useInput(3.3);
  const { value: quiescentMicroA, bind: bindQuiescentMicroA } = useInput(55);
  const { value: otherQuiescentMicroA, bind: bindOtherQuiescentMicroA } =
    useInput(0);
  const { value: efficiency, bind: bindEfficiency } = useInput(0.9);

  const { value: bondedQty, bind: bindBondedQty } = useInput(1);
  const { value: percentAsleep, bind: bindPercentAsleep } = useInput(0.5);

  const { value: glowEnabled, bind: bindGlowEnabled } = useInput(false);
  const { value: glowQuantity, bind: bindGlowQuantity } = useInput(10);
  const { value: glowBrightness, bind: bindGlowBrightness } = useInput(1);

  const { value: displayEnabled, bind: bindDisplayEnabled } = useInput(false);
  const { value: displayType, bind: bindDisplayType } = useInput("");

  const [disclaimerAcknowledged, setDisclaimerAcknowledged] = useState(
    typeof window !== "undefined"
      ? localStorage.getItem("zmkPowerProfilerDisclaimer") === "true"
      : false
  );

  const currentBoard =
    board === "custom"
      ? {
          powerSupply: {
            type: psuType,
            outputVoltage: outputV,
            quiescentMicroA: quiescentMicroA,
            efficiency,
          },
          otherQuiescentMicroA: otherQuiescentMicroA,
        }
      : zmkBoards[board];

  return (
    <Layout
      title={`ZMK Power Profiler`}
      description="Estimate your keyboard's power usage and battery life on ZMK."
    >
      <header className={classnames("hero hero--primary", styles.heroBanner)}>
        <div className="container">
          <h1 className="hero__title">ZMK Power Profiler</h1>
          <p className="hero__subtitle">
            {"Estimate your keyboard's power usage and battery life on ZMK."}
          </p>
        </div>
      </header>
      <main>
        <section className="container">
          <div className="profilerSection">
            <h3>Keyboard Specifications</h3>
            <div className="row">
              <div className="col col--4">
                <div className="profilerInput">
                  <label>Board</label>
                  <select {...bindBoard}>
                    <option hidden value="">
                      Select a board
                    </option>
                    {Object.keys(zmkBoards).map((b) => (
                      <option key={b}>{b}</option>
                    ))}
                    <option value="custom">Custom</option>
                  </select>
                </div>
              </div>
              <div className="col col--4">
                <div className="profilerInput">
                  <label>Split Keyboard</label>
                  <input
                    id="split"
                    checked={split}
                    {...bindSplit}
                    className="toggleInput"
                    type="checkbox"
                  />
                  <label htmlFor="split" className="toggle">
                    <div className="toggleThumb" />
                  </label>
                </div>
              </div>
              <div className="col col--4">
                <div className="profilerInput">
                  <label>Battery Size</label>
                  <div className="inputBox">
                    <input {...bindBatteryMilliAh} type="number" />
                    <span>mAh</span>
                  </div>
                </div>
              </div>
            </div>
          </div>

          {board === "custom" && (
            <CustomBoardForm
              bindPsuType={bindPsuType}
              bindOutputV={bindOutputV}
              bindEfficiency={bindEfficiency}
              bindQuiescentMicroA={bindQuiescentMicroA}
              bindOtherQuiescentMicroA={bindOtherQuiescentMicroA}
            />
          )}

          <div className="profilerSection">
            <h3>Usage Values</h3>
            <div className="row">
              <div className="col col--4">
                <div className="profilerInput">
                  <label>
                    Bonded Bluetooth Profiles{" "}
                    <span tooltip="The average number of host devices connected at once">
                      ⓘ
                    </span>
                  </label>
                  <input {...bindBondedQty} type="range" min="1" max="5" />
                  <span>{bondedQty}</span>
                </div>
              </div>
              <div className="col col--4">
                <div className="profilerInput">
                  <label>
                    Percentage Asleep{" "}
                    <span tooltip="How much time the keyboard is in deep sleep (15 min. default timeout)">
                      ⓘ
                    </span>
                  </label>
                  <input
                    {...bindPercentAsleep}
                    type="range"
                    min="0"
                    step=".1"
                    max="1"
                  />
                  <span>{Math.round(percentAsleep * 100)}%</span>
                </div>
              </div>
            </div>
          </div>

          <div className="profilerSection">
            <h3>Features</h3>
            <div className="row">
              <div className="col col--4">
                <div className="profilerInput">
                  <label>RGB Underglow</label>
                  <input
                    checked={glowEnabled}
                    id="glow"
                    {...bindGlowEnabled}
                    className="toggleInput"
                    type="checkbox"
                  />
                  <label htmlFor="glow" className="toggle">
                    <div className="toggleThumb" />
                  </label>
                </div>
                {glowEnabled && (
                  <>
                    <div className="profilerInput">
                      <label>LED Quantity</label>
                      <div className="inputBox">
                        <input {...bindGlowQuantity} type="number" />
                      </div>
                    </div>
                    <div className="profilerInput">
                      <label>Brightness</label>
                      <input
                        {...bindGlowBrightness}
                        type="range"
                        min="0"
                        step=".01"
                        max="1"
                      />
                      <span>{Math.round(glowBrightness * 100)}%</span>
                    </div>
                  </>
                )}
              </div>
              <div className="col col--4">
                <div className="profilerInput">
                  <label>Display</label>
                  <input
                    checked={displayEnabled}
                    id="display"
                    {...bindDisplayEnabled}
                    className="toggleInput"
                    type="checkbox"
                  />
                  <label htmlFor="display" className="toggle">
                    <div className="toggleThumb" />
                  </label>
                </div>
                {displayEnabled && (
                  <div className="profilerInput">
                    <label>Display Type</label>
                    <select {...bindDisplayType}>
                      <option hidden selected>
                        Select type
                      </option>
                      <option value="EPAPER">ePaper</option>
                      <option value="OLED">OLED</option>
                    </select>
                  </div>
                )}
              </div>
            </div>
          </div>
          {split ? (
            <>
              <PowerEstimate
                board={currentBoard}
                splitType="central"
                batteryMilliAh={batteryMilliAh}
                usage={{ bondedQty, percentAsleep }}
                underglow={{ glowEnabled, glowBrightness, glowQuantity }}
                display={{ displayEnabled, displayType }}
              />
              <PowerEstimate
                board={currentBoard}
                splitType="peripheral"
                batteryMilliAh={batteryMilliAh}
                usage={{ bondedQty, percentAsleep }}
                underglow={{ glowEnabled, glowBrightness, glowQuantity }}
                display={{ displayEnabled, displayType }}
              />
            </>
          ) : (
            <PowerEstimate
              board={currentBoard}
              splitType="standalone"
              batteryMilliAh={batteryMilliAh}
              usage={{ bondedQty, percentAsleep }}
              underglow={{ glowEnabled, glowBrightness, glowQuantity }}
              display={{ displayEnabled, displayType }}
            />
          )}
          <div className="row">
            <div className="col col--8 col--offset-2 profilerDisclaimer">
              Disclaimer: {Disclaimer}
            </div>
          </div>
        </section>
      </main>
      {!disclaimerAcknowledged && (
        <div className="disclaimerHolder">
          <div className="disclaimer">
            <h3>Disclaimer</h3>
            <p>{Disclaimer}</p>
            <button
              onClick={() => {
                setDisclaimerAcknowledged(true);
                localStorage.setItem("zmkPowerProfilerDisclaimer", true);
              }}
            >
              I Understand
            </button>
          </div>
        </div>
      )}
    </Layout>
  );
}

export default PowerProfiler;
