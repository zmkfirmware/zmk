/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

import React from "react";
import PropTypes from "prop-types";
import {
  displayPower,
  underglowPower,
  backlightPower,
  zmkBase,
} from "../data/power";
import "../css/power-estimate.css";

// Average monthly discharge percent
const lithiumIonMonthlyDischargePercent = 5;
// Average voltage of a lithium ion battery based of discharge graphs
const lithiumIonAverageVoltage = 3.8;
// Average discharge efficiency of li-ion https://en.wikipedia.org/wiki/Lithium-ion_battery
const lithiumIonDischargeEfficiency = 0.85;
// Range of the discharge efficiency
const lithiumIonDischargeEfficiencyRange = 0.05;

// Proportion of time spent typing (keys being pressed down and scanning). Estimated to 2%.
const timeSpentTyping = 0.02;

// Nordic power profiler kit accuracy
const measurementAccuracy = 0.2;

const batVolt = lithiumIonAverageVoltage;

const palette = [
  "#bbdefb",
  "#90caf9",
  "#64b5f6",
  "#42a5f5",
  "#2196f3",
  "#1e88e5",
  "#1976d2",
];

function formatUsage(microWatts) {
  if (microWatts > 1000) {
    return (microWatts / 1000).toFixed(1) + "mW";
  }

  return Math.round(microWatts) + "µW";
}

function voltageEquivalentCalc(powerSupply) {
  if (powerSupply.type === "LDO") {
    return batVolt;
  } else if (powerSupply.type === "SWITCHING") {
    return powerSupply.outputVoltage / powerSupply.efficiency;
  }
}

function formatMinutes(minutes, precision, floor) {
  let message = "";
  let count = 0;

  let units = ["year", "month", "week", "day", "hour", "minute"];
  let multiples = [60 * 24 * 365, 60 * 24 * 30, 60 * 24 * 7, 60 * 24, 60, 1];

  for (let i = 0; i < units.length; i++) {
    if (minutes >= multiples[i]) {
      const timeCount = floor
        ? Math.floor(minutes / multiples[i])
        : Math.ceil(minutes / multiples[i]);
      minutes -= timeCount * multiples[i];
      count++;
      message +=
        timeCount + (timeCount > 1 ? ` ${units[i]}s ` : ` ${units[i]} `);
    }

    if (count == precision) return message;
  }

  return message || "0 minutes";
}

function PowerEstimate({
  board,
  splitType,
  batteryMilliAh,
  usage,
  underglow,
  backlight,
  display,
}) {
  if (!board || !board.powerSupply.type || !batteryMilliAh) {
    return (
      <div className="powerEstimate">
        <h3>
          <span>{splitType !== "standalone" ? splitType + ": " : " "}...</span>
        </h3>
        <div className="powerEstimateBar">
          <div
            className="powerEstimateBarSection"
            style={{
              width: "100%",
              background: "#e0e0e0",
              mixBlendMode: "overlay",
            }}
          ></div>
        </div>
      </div>
    );
  }

  const powerUsage = [];
  let totalUsage = 0;

  const voltageEquivalent = voltageEquivalentCalc(board.powerSupply);

  // Lithium ion self discharge
  const lithiumMonthlyDischargemAh =
    parseInt(batteryMilliAh) * (lithiumIonMonthlyDischargePercent / 100);
  const lithiumDischargeMicroA = (lithiumMonthlyDischargemAh * 1000) / 30 / 24;
  const lithiumDischargeMicroW = lithiumDischargeMicroA * batVolt;

  totalUsage += lithiumDischargeMicroW;
  powerUsage.push({
    title: "Battery Self Discharge",
    usage: lithiumDischargeMicroW,
  });

  // Quiescent current
  const quiescentMicroATotal =
    parseInt(board.powerSupply.quiescentMicroA) +
    parseInt(board.otherQuiescentMicroA);
  const quiescentMicroW = quiescentMicroATotal * voltageEquivalent;

  totalUsage += quiescentMicroW;
  powerUsage.push({
    title: "Board Quiescent Usage",
    usage: quiescentMicroW,
  });

  // ZMK overall usage
  const zmkMicroA =
    zmkBase[splitType].idle +
    (splitType !== "peripheral" ? zmkBase.hostConnection * usage.bondedQty : 0);

  const zmkMicroW = zmkMicroA * voltageEquivalent;
  const zmkUsage = zmkMicroW * (1 - usage.percentAsleep);

  totalUsage += zmkUsage;
  powerUsage.push({
    title: "ZMK Base Usage",
    usage: zmkUsage,
  });

  // ZMK typing usage
  const zmkTypingMicroA = zmkBase[splitType].typing * timeSpentTyping;

  const zmkTypingMicroW = zmkTypingMicroA * voltageEquivalent;
  const zmkTypingUsage = zmkTypingMicroW * (1 - usage.percentAsleep);

  totalUsage += zmkTypingUsage;
  powerUsage.push({
    title: "ZMK Typing Usage",
    usage: zmkTypingUsage,
  });

  if (underglow.glowEnabled) {
    const underglowAverageLedMicroA =
      underglow.glowBrightness *
        (underglowPower.ledOn - underglowPower.ledOff) +
      underglowPower.ledOff;

    const underglowMicroA =
      underglowPower.firmware +
      underglow.glowQuantity * underglowAverageLedMicroA;

    const underglowMicroW = underglowMicroA * voltageEquivalent;

    const underglowUsage = underglowMicroW * (1 - usage.percentAsleep);

    totalUsage += underglowUsage;
    powerUsage.push({
      title: "RGB Underglow",
      usage: underglowUsage,
    });
  }

  if (backlight.backlightEnabled) {
    let backlightMicroA =
      ((board.powerSupply.outputVoltage - backlight.backlightVoltage) /
        backlight.backlightResistance) *
      1000000 *
      backlight.backlightBrightness *
      backlight.backlightQuantity;

    if (
      backlight.backlightBrightness > 0 &&
      backlight.backlightBrightness < 1
    ) {
      backlightMicroA += backlightPower.pwmPower;
    }

    const backlightMicroW = backlightMicroA * voltageEquivalent;
    const backlightUsage = backlightMicroW * (1 - usage.percentAsleep);

    totalUsage += backlightUsage;
    powerUsage.push({
      title: "Backlight",
      usage: backlightUsage,
    });
  }

  if (display.displayEnabled && display.displayType) {
    const { activePercent, active, sleep } = displayPower[display.displayType];

    const displayMicroA = active * activePercent + sleep * (1 - activePercent);
    const displayMicroW = displayMicroA * voltageEquivalent;
    const displayUsage = displayMicroW * (1 - usage.percentAsleep);

    totalUsage += displayUsage;
    powerUsage.push({
      title: "Display",
      usage: displayUsage,
    });
  }

  // Calculate the average minutes of use
  const estimatedAvgEffectiveMicroWH =
    batteryMilliAh * batVolt * lithiumIonDischargeEfficiency * 1000;

  const estimatedAvgMinutes = Math.round(
    (estimatedAvgEffectiveMicroWH / totalUsage) * 60
  );

  // Calculate worst case for battery life
  const worstLithiumIonDischargeEfficiency =
    lithiumIonDischargeEfficiency - lithiumIonDischargeEfficiencyRange;

  const estimatedWorstEffectiveMicroWH =
    batteryMilliAh * batVolt * worstLithiumIonDischargeEfficiency * 1000;

  const highestTotalUsage = totalUsage * (1 + measurementAccuracy);

  const estimatedWorstMinutes = Math.round(
    (estimatedWorstEffectiveMicroWH / highestTotalUsage) * 60
  );

  // Calculate range (+-) of minutes using average - worst
  const estimatedRange = estimatedAvgMinutes - estimatedWorstMinutes;

  return (
    <div className="powerEstimate">
      <h3>
        <span>{splitType !== "standalone" ? splitType + ": " : " "}</span>
        {formatMinutes(estimatedAvgMinutes, 2, true)} (±
        {formatMinutes(estimatedRange, 1, false).trim()})
      </h3>
      <div className="powerEstimateBar">
        {powerUsage.map((p, i) => (
          <div
            key={p.title}
            className={
              "powerEstimateBarSection" + (i > 1 ? " rightSection" : "")
            }
            style={{
              width: (p.usage / totalUsage) * 100 + "%",
              background: palette[i],
            }}
          >
            <div className="powerEstimateTooltipWrap">
              <div className="powerEstimateTooltip">
                <div>
                  {p.title} - {Math.round((p.usage / totalUsage) * 100)}%
                </div>
                <div style={{ fontSize: ".875rem" }}>
                  ~{formatUsage(p.usage)} estimated avg. consumption
                </div>
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}

PowerEstimate.propTypes = {
  board: PropTypes.Object,
  splitType: PropTypes.string,
  batteryMilliAh: PropTypes.number,
  usage: PropTypes.Object,
  underglow: PropTypes.Object,
  backlight: PropTypes.Object,
  display: PropTypes.Object,
};

export default PowerEstimate;
